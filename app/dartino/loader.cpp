// Copyright (c) 2015, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include "loader.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#include <platform.h>
#include <kernel/thread.h>
#include <lib/fs.h>

#include <lib/bio.h>
#include <lib/page_alloc.h>
#include <lib/tftp.h>

#include <include/dartino_api.h>

#include "generate_flashtool_args.h"

const size_t kDownloadSlotSize = (512 * 1024);
const size_t kFnameSize = 32;

namespace {

DartinoProgramGroup ram_programs = 0;
DartinoProgramGroup flash_programs = 0;

// The below should be thread safe is this was for real...
static int freeze_requests = 0;

static void SetupLinearMode(const char* device) {
  bdev_t* bd = bio_open(device);
  if (!bd) {
    printf("error opening %s\n", device);
    abort();
  }
  unsigned char* unused = 0;
  int rv = bio_ioctl(bd, BIO_IOCTL_GET_MEM_MAP, &unused);
  if (rv < 0) {
    printf("error %d in %s ioctl\n", rv, device);
    abort();
  }
  bio_close(bd);
}

void SwitchFromLinearMode(const char* device) {
  if (freeze_requests++ > 0) {
    printf("Already in write-to-flash mode!\n");
  } else {
    bdev_t* bd = bio_open(device);
    if (flash_programs) DartinoFreezeProgramGroup(flash_programs);
    int rv = bio_ioctl(bd, BIO_IOCTL_PUT_MEM_MAP, NULL);
    if (rv < 0) {
      printf("error %d in %s ioctl\n", rv, device);
      abort();
    }
    bio_close(bd);
  }
}

void SwitchToLinearMode(const char* device) {
  if (--freeze_requests > 0) {
    printf("Still in run-from-flash mode!\n");
  } else {
    if (flash_programs) DartinoUnfreezeProgramGroup(flash_programs);
    SetupLinearMode(device);
  }
}

static bool IsInLinearMode(void) {
  return freeze_requests == 0;
}

typedef struct {
  unsigned char* start;
  unsigned char* end;
  unsigned char* max;
  char name[kFnameSize];
} download_t;

download_t* MakeDownload(const char* name) {
  download_t* d = new download_t;

  // use the page alloc api to grab space for the app.
  d->start = reinterpret_cast<unsigned char*>(
      page_alloc(kDownloadSlotSize / PAGE_SIZE, PAGE_ALLOC_ANY_ARENA));
  if (!d->start) {
    delete d;
    printf("error allocating memory for download\n");
    return NULL;
  }

  d->end = d->start;
  d->max = d->end + kDownloadSlotSize;

  strncpy(d->name, name, kFnameSize);
  memset(d->start, 0, kDownloadSlotSize);
  return d;
}

static int LiveDebug(void* ctx) {
  int port = *reinterpret_cast<int*>(ctx);

  printf("ready for dartino debug via port: %i\n", port);
  while (true) {
    DartinoWaitForDebuggerConnection(port);
  }

  return 0;
}

static void RunFinishedCallback(DartinoProgram program, int exitcode,
                                void* data) {
  lk_bigtime_t start = reinterpret_cast<lk_bigtime_t>(data);
  lk_bigtime_t elapsed = current_time_hires() - start;
  printf("dartino-vm ran for %llu usecs, returned %d\n", elapsed, exitcode);
}

typedef struct {
  download_t* download;
  DartinoProgram program;
} program_gen_msg_t;

static ssize_t GetFileSize(filehandle* handle) {
  struct file_stat stats;
  if (fs_stat_file(handle, &stats) != NO_ERROR) return 0;
  return stats.size;
}

static ssize_t GetFileCapacity(filehandle* handle) {
  struct file_stat stats;
  if (fs_stat_file(handle, &stats) != NO_ERROR) return 0;
  return stats.capacity;
}

static int ProgramGenerator(void* arg) {
  program_gen_msg_t* data = reinterpret_cast<program_gen_msg_t*>(arg);
  int len = (data->download->end - data->download->start);

  printf("loading snapshot: %d bytes ...\n", len);
  data->program = DartinoLoadSnapshot(data->download->start, len);

  thread_exit(0);
}

int StartSnapshotFromDownload(download_t* download) {
  program_gen_msg_t msg;
  msg.download = download;

  // Offload the snapshot loading to a separate thread to increase stack size.
  thread_t* generator =
      thread_create("prggen", &ProgramGenerator, &msg, DEFAULT_PRIORITY, 8192);
  thread_resume(generator);
  thread_join(generator, NULL, INFINITE_TIME);

  printf("running program...\n");

  lk_bigtime_t start = current_time_hires();
  DartinoAddProgramToGroup(ram_programs, msg.program);
  DartinoStartMain(msg.program, &RunFinishedCallback,
                   reinterpret_cast<void*>(start), 0, NULL);

  return 0;
}

void Burn(const char* device, download_t* download) {
  // Validate download->name.
  if (download->name == NULL) {
    printf("Download name is null. Aborting.\n");
    return;
  }

  if (strstr(download->name, "/") != NULL) {
    printf("Download name may not contain a '/' Download name is %s. Aborting",
           download->name);
    return;
  }

  // Create an install path for the new binary.
  char install_path[kFnameSize];
  snprintf(install_path, kFnameSize, "%s/%s", "/spifs",
           download->name);
  printf("Installing '%s' to '%s'\n", download->name, install_path);

  ssize_t len = download->end - download->start;

  SwitchFromLinearMode(device);

  filehandle* handle;
  status_t result = fs_create_file(
      install_path,
      &handle,
      len
  );

  ssize_t bytes;
  if (result == ERR_ALREADY_EXISTS) {
    result = fs_open_file(install_path, &handle);
    if (result != NO_ERROR) {
      printf("A program already exists at this path but the file cannot be\n"
             "opened for reading [%d]...\n", result);
      goto exit;
    }
    if (GetFileCapacity(handle) < len) {
      printf("A program already exists at %s but the file is too small.\n",
             install_path);
      goto exit;
    }
    if (GetFileSize(handle) > len) {
      result = fs_truncate_file(handle, len);
      if (result != NO_ERROR) {
        printf("Truncating %s failed with %d.\n", install_path, result);
        goto exit;
      }
    }
  } else if (result != NO_ERROR) {
    printf("Creating %s failed with error %d. Aborting.\n", install_path,
           result);
    goto exit;
  } else {
    printf("Created a file at %s\n", install_path);
  }

  bytes = fs_write_file(handle, download->start, 0, len);
  if (bytes != len) {
    printf("Could not write snapshot data. Expected to write %ld bytes "
           "wrote %ld bytes instead. Aborting.\n", len, bytes);
    goto exit;
  }

  printf("Wrote a %ld byte payload for %s\n", bytes, install_path);

exit:
  result = fs_close_file(handle);
  if (result != NO_ERROR) {
    printf("Error closing file. Status code = %d\n", result);
  }
  SwitchToLinearMode(device);
}

void FlashRun(const char* device, const char* filename) {
  filehandle* handle;
  status_t result = fs_open_file(filename, &handle);
  if (result != NO_ERROR) {
    printf("Open file %s failed with status = %d. Aborting.\n",
           filename, result);
    return;
  }

  unsigned char* address = 0;
  result = fs_file_ioctl(handle, FS_IOCTL_GET_FILE_ADDR, &address);
  if (result != NO_ERROR) {
    printf("Failed to get memory mapped address for %s. Status code = %d\n",
           filename, result);
    // TODO(gkalsi): Close the file?
  }
  size_t len = GetFileSize(handle);
  if (!len) {
    printf("invalid snapshot length\n");
    return;
  }

  fs_close_file(handle);

  if (!IsInLinearMode()) {
    printf("Trying to start a program while not in linear mode. Try again...");
    return;
  }

  printf("data at %p is %d bytes\n", address, len);

  // First try to run this directly as a program heap
  DartinoProgram program = DartinoLoadProgramFromFlash(address, len);
  if (program != NULL) {
    DartinoAddProgramToGroup(flash_programs, program);
    printf("running program (heap-blob)...\n");
    lk_bigtime_t start = current_time_hires();
    DartinoStartMain(program, &RunFinishedCallback,
                     reinterpret_cast<void*>(start), 0, NULL);
  } else {
    download_t* download = new download_t;
    download->start = address;
    download->end = download->start + len;
    strncpy(download->name, filename, kFnameSize);

    StartSnapshotFromDownload(download);
  }
  return;
}

int TftpCallback(void* data, size_t len, download_t* download) {
  if (!data)
    return -1;

  if ((download->end + len) > download->max) {
    printf("transfer too big, aborting\n");
    return -1;
  }

  if (len) {
    memcpy(download->end, data, len);
    download->end += len;
  }
  return 0;
}

int LoadAndRunCallback(void* data, size_t len, void* arg) {
  download_t* download = reinterpret_cast<download_t*>(arg);
  if (!data) {
    StartSnapshotFromDownload(download);
    return 0;
  }
  return TftpCallback(data, len, download);
}

int LoadAndBurnCallback(void* data, size_t len, void* arg) {
  download_t* download = reinterpret_cast<download_t*>(arg);
  if (!data) {
    Burn("qspi-flash", download);
    return 0;
  }
  return TftpCallback(data, len, download);
}

}  // namespace

void LoaderInit() {
  tftp_server_init(NULL);
  SetupLinearMode("qspi-flash");
}

int LoadSnapshotFromNetwork(const char* name) {
  download_t* download = MakeDownload(name);
  if (!download)
    return -1;
  return tftp_set_write_client(download->name, &LoadAndRunCallback, download);
}

int AddSnapshotToFlash(const char* name) {
 download_t* download = MakeDownload(name);
  if (!download)
    return -1;
  return tftp_set_write_client(download->name, &LoadAndBurnCallback, download);
}

int LoadSnapshotFromFlash(const char* name) {
  FlashRun("qspi-flash", name);
  return 0;
}

int DebugSnapshot(int port) {
    thread_resume(
      thread_create("dartino dbg-vm", &LiveDebug, new int(port),
                    DEFAULT_PRIORITY, 8192));
  return 0;
}

void PrepareBlob(const char* file, const char* size_str) {
  if (strstr(file, "/") != NULL) {
    printf("Blob name may not contain a '/' Blob name is %s. Aborting", file);
    return;
  }
  size_t len = atoi(size_str);
  if (len <= 0) {
    printf("Illegal size, parsed %s as %d. Aborting.", size_str, len);
    return;
  }

  // Create an install path for the new binary.
  char install_path[kFnameSize];
  snprintf(install_path, kFnameSize, "%s/%s", "/spifs", file);
  printf("Installing '%s' to '%s'\n", file, install_path);

  SwitchFromLinearMode("qspi-flash");

  filehandle* handle = 0;
  unsigned char* address = 0;
  status_t result = fs_create_file(
      install_path,
      &handle,
      len
  );

  if (result == ERR_ALREADY_EXISTS) {
    printf("A program already exists at this path. Please use another name or "
           "delete the existing program and try again.\n");
    goto exit;
  } else if (result != NO_ERROR) {
    printf("Creating a file failed with error %d. Aborting.\n", result);
    goto exit;
  }

  result = fs_truncate_file(handle, 0);
  if (result != NO_ERROR) {
    printf("Truncating the file failed with %d.\n", result);
  }
  printf("Created a file at %s\n", install_path);

  result = fs_file_ioctl(handle, FS_IOCTL_GET_FILE_ADDR, &address);
  if (result != NO_ERROR) {
    printf("Failed to get memory mapped address for %s. Status code = %d\n",
           file, result);
    goto exit;
  }

  printf("Now create a blob using flashtool, the command line is:\n\n");

  {
    const int kBufferSize = 256;
    char* buffer = reinterpret_cast<char*>(malloc(kBufferSize));
    GenerateFlashtoolArgs(buffer, kBufferSize, address, file);
    printf("flashtool %s\n\n", buffer);
    free(buffer);
  }

exit:
  if (handle) {
    result = fs_close_file(handle);
    if (result != NO_ERROR) {
      printf("Failed to close file %s. Error was %d.\n", file, result);
    }
  }

  SwitchToLinearMode("qspi-flash");
}

void InitializeGroups() {
  ram_programs = DartinoCreateProgramGroup("in_ram");
  flash_programs = DartinoCreateProgramGroup("in_flash");
  DartinoFreezeProgramGroup(flash_programs);
}

void DestroyGroups() {
  DartinoDeleteProgramGroup(ram_programs);
  DartinoDeleteProgramGroup(flash_programs);
}

void FreezeVM() {
  SwitchFromLinearMode("qspi-flash");
}

void UnfreezeVM() {
  SwitchToLinearMode("qspi-flash");
}

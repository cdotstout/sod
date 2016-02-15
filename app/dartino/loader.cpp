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

const size_t kDownloadSlotSize = (512 * 1024);
const size_t kFnameSize = 32;

namespace {

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
      page_alloc(kDownloadSlotSize / PAGE_SIZE));
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

static void RunFinishedCallback(DartinoProgram* program, int exitcode,
                                void* data) {
  lk_bigtime_t start = reinterpret_cast<lk_bigtime_t>(data);
  lk_bigtime_t elapsed = current_time_hires() - start;
  printf("dartino-vm ran for %llu usecs, returned %d\n", elapsed, exitcode);
}

typedef struct {
  download_t* download;
  DartinoProgram program;
} program_gen_msg_t;

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
  DartinoStartMain(msg.program, &RunFinishedCallback,
                   reinterpret_cast<void*>(start));

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

  filehandle *handle;
  status_t result = fs_create_file(
      install_path,
      &handle,
      len + sizeof(len)
  );

  if (result == ERR_ALREADY_EXISTS) {
    printf("A program already exists at this path. Please use another name or "
           "delete the existing program and try again.\n");
    return;
  } else if (result != NO_ERROR) {
    printf("Creating a file failed with error %d. Aborting.\n", result);
    return;
  }

  printf("Created a file at %s\n", install_path);

  ssize_t bytes = fs_write_file(handle, &len, 0, sizeof(len));
  if (bytes != sizeof(len)) {
    printf("Could not write header for binary. Expected to write %d bytes "
           "wrote %ld bytes instead. Aborting.\n", sizeof(len), bytes);
    goto exit;
  }

  printf("Wrote a %ld byte header for %s\n", bytes, install_path);

  bytes = fs_write_file(handle, download->start, sizeof(len), len);
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
}

void FlashRun(const char* device, const char* filename) {
  filehandle *handle;
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

  bdev_t *bd = bio_open(device);
  if (!bd) {
    printf("error opening %s\n", device);
    return;
  }
  unsigned char* unused = 0;
  int rv = bio_ioctl(bd, BIO_IOCTL_GET_MEM_MAP, &unused);
  if (rv < 0) {
    printf("error %d in %s ioctl\n", rv, device);
    return;
  }

  size_t len = *((size_t*)address);
  if (!len) {
    printf("invalid snapshot length\n");
    return;
  }
  address += sizeof(len);
  printf("snapshot at %p is %d bytes\n", address, len);

  download_t* download = new download_t;
  download->start = address;
  download->end = download->start + len;
  strncpy(download->name, filename, kFnameSize);

  StartSnapshotFromDownload(download);
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

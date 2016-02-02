// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include "loader.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#include <platform.h>
#include <kernel/thread.h>
#include <target/fsconfig.h>
#include <lib/fs/spifs.h>

#include <lib/bio.h>
#include <lib/page_alloc.h>
#include <lib/tftp.h>

#include <include/fletch_api.h>

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
  printf("starting fletch-vm...\n");
  FletchSetup();

  printf("ready for fletch debug via port: %i\n", port);
  while (true) {
    FletchWaitForDebuggerConnection(port);
  }

  FletchTearDown();
  return 0;
}

int RunSnapshot(void* ctx) {
  download_t* d = reinterpret_cast<download_t*>(ctx);

  printf("starting fletch-vm...\n");
  FletchSetup();

  int len = (d->end - d->start);
  printf("loading snapshot: %d bytes ...\n", len);
  FletchProgram program = FletchLoadSnapshot(d->start, len);

  printf("running program...\n");

  lk_bigtime_t start = current_time_hires();
  int result = FletchRunMain(program);

  lk_bigtime_t elapsed = current_time_hires() - start;
  printf("fletch-vm ran for %llu usecs\n", elapsed);

  FletchDeleteProgram(program);

  printf("tearing down fletch-vm...\n");
  printf("vm exit code: %i\n", result);
  FletchTearDown();

  return result;
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
  snprintf(install_path, kFnameSize, "%s/%s", SPIFS_MOUNT_POINT,
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

static void LiveRun(download_t* download) {
  thread_resume(
      thread_create("fletch vm", &RunSnapshot, download,
                    DEFAULT_PRIORITY, 8192));
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

  LiveRun(download);
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
    LiveRun(download);
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
      thread_create("fletch dbg-vm", &LiveDebug, new int(port),
                    DEFAULT_PRIORITY, 8192));
  return 0;
}

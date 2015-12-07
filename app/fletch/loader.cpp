// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include "loader.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <platform.h>
#include <kernel/thread.h>

#include <lib/bio.h>
#include <lib/page_alloc.h>
#include <lib/tftp.h>

#include <include/fletch_api.h>

#define DOWNLOAD_SLOT_SIZE (512 * 1024)

#define FNAME_SIZE 64

namespace {

typedef struct {
  unsigned char* start;
  unsigned char* end;
  unsigned char* max;
  char name[FNAME_SIZE];
} download_t;

download_t* MakeDownload(const char* name) {
  download_t* d = new download_t;

  // use the page alloc api to grab space for the app.
  d->start = reinterpret_cast<unsigned char*>(
      page_alloc(DOWNLOAD_SLOT_SIZE / PAGE_SIZE));
  if (!d->start) {
    delete d;
    printf("error allocating memory for download\n");
    return NULL;
  }

  d->end = d->start;
  d->max = d->end + DOWNLOAD_SLOT_SIZE;

  strncpy(d->name, name, FNAME_SIZE);
  memset(d->start, 0, DOWNLOAD_SLOT_SIZE);
  return d;
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
  printf("writting to %s ..\n", device);
  bdev_t *bd = bio_open(device);
  if (!bd) {
    printf("can't open %s\n", device);
    return;
  }

  size_t len = download->end - download->start;

  ssize_t rv = bio_erase(bd, 0, len + sizeof(len));
  if (rv < 0) {
    printf("error %ld erasing %s\n", rv, device);
    return; 
  }

  rv = bio_write(bd, &len, 0, sizeof(len));
  rv = bio_write(bd, download->start, sizeof(len), len);
  bio_close(bd);

  if (rv < 0) {
    printf("error %ld while writting to %s\n", rv, device);    
    return;
  }
  printf("%ld bytes written to %s\n", rv, device);
}

static void LiveRun(download_t* download) {
  thread_resume(
      thread_create("fletch vm", &RunSnapshot, download,
                    DEFAULT_PRIORITY, 8192));
}

void FlashRun(const char* device) {
  bdev_t *bd = bio_open(device);
  if (!bd) {
    printf("error opening %s\n", device);
    return;
  }
  unsigned char* address = 0;
  int rv = bio_ioctl(bd, BIO_IOCTL_GET_MEM_MAP, &address);
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
  strncpy(download->name, "flash", FNAME_SIZE);

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

static void Debug(int port) {
  printf("starting fletch-vm...\n");
  FletchSetup();

  printf("wainting for debug connection on port: %i\n", port);
  FletchWaitForDebuggerConnection(port);

  printf("vm exit");
  FletchTearDown();
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
  FlashRun("qspi-flash");
  return 0;
}

int DebugSnapshot(int port) {
  Debug(port);
  return 0;
}

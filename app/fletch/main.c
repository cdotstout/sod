// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include <app.h>
#include <include/fletch_api.h>
#include <include/static_ffi.h>

#include <endian.h>
#include <platform.h>
#include <kernel/thread.h>
#include <dev/display.h>

#include <lib/bio.h>
#include <lib/font.h>
#include <lib/gfx.h>
#include <lib/tftp.h>
#include <lib/page_alloc.h>

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>
#else
#error "fletch app needs a console"
#endif

#define DOWNLOAD_SLOT_SIZE (512 * 1024)

#define FNAME_SIZE 64

//////////////// TFTP & Dart ////////////////////////////////////////////////

enum {
  MODE_RUN,
  MODE_BURN
} fletch_mode;


typedef struct {
  unsigned char* start;
  unsigned char* end;
  unsigned char* max;
  char name[FNAME_SIZE];
} download_t;

static download_t* MakeDownload(const char* name) {
  download_t* d = malloc(sizeof(download_t));

  // use the page alloc api to grab space for the app
  d->start = page_alloc(DOWNLOAD_SLOT_SIZE / PAGE_SIZE);
  if (!d->start) {
    free(d);
    printf("error allocating memory for download\n");
    return NULL;
  }

  d->end = d->start;
  d->max = d->end + DOWNLOAD_SLOT_SIZE;

  strncpy(d->name, name, FNAME_SIZE);
  memset(d->start, 0, DOWNLOAD_SLOT_SIZE);
  return d;
}

static int RunSnapshot(void* ctx) {
  download_t* d = ctx;

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

static void Debug(int port) {
  printf("starting fletch-vm...\n");
  FletchSetup();

  printf("wainting for debug connection on port: %i\n", port);
  FletchWaitForDebuggerConnection(port);

  printf("vm exit");
  FletchTearDown();
}

static void LiveRun(download_t* download) {
  thread_resume(
      thread_create("fletch vm", &RunSnapshot, download,
                    DEFAULT_PRIORITY, 8192));
}

static void Burn(const char* device, download_t* download) {
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

static void FlashRun(const char* device) {
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

  download_t* download = malloc(sizeof(download_t));
  download->start = address;
  download->end = download->start + len;
  strncpy(download->name, "flash", FNAME_SIZE);

  LiveRun(download);
  return;  
}

int TftpCallback(void* data, size_t len, void* arg) {
  download_t* download = arg;

  if (!data) {
    // Done with the download. 
    if (fletch_mode == MODE_BURN) {
      // Write to QSPI flash.
      Burn("qspi-flash", download);
    } else {
      // Run the snapshot in a separate thread.
      LiveRun(download);  
    }
    return 0;
  }

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

//////////////// Dart FFI setup ///////////////////////////////////////////////

static gfx_surface* GetFullscreenSurface(void) {
  struct display_info info;
  display_get_info(&info);
  return gfx_create_surface_from_display(&info);
}

static int GetWidth(gfx_surface* surface) {
  return surface->width;
}

static int GetHeight(gfx_surface* surface) {
  return surface->height;
}

#define FFI_TABLE_SIZE 10

static int GetFFITableSize(void) {
  return FFI_TABLE_SIZE;
}

FLETCH_EXPORT_TABLE_BEGIN
FLETCH_EXPORT_TABLE_ENTRY("ffi_table", GetFFITableSize)
FLETCH_EXPORT_TABLE_ENTRY("gfx_create", GetFullscreenSurface)
FLETCH_EXPORT_TABLE_ENTRY("gfx_width", GetWidth)
FLETCH_EXPORT_TABLE_ENTRY("gfx_height", GetHeight)
FLETCH_EXPORT_TABLE_ENTRY("gfx_destroy", gfx_surface_destroy)
FLETCH_EXPORT_TABLE_ENTRY("gfx_pixel", gfx_putpixel)
FLETCH_EXPORT_TABLE_ENTRY("gfx_line", gfx_line)
FLETCH_EXPORT_TABLE_ENTRY("gfx_clear", gfx_clear)
FLETCH_EXPORT_TABLE_ENTRY("gfx_flush", gfx_flush)
FLETCH_EXPORT_TABLE_ENTRY("font_draw_char", font_draw_char)
FLETCH_EXPORT_TABLE_END

//////////////// Shell handler ///////////////////////////////////////////////

static int FletchRunner(int argc, const cmd_args* argv) {
  if ((argc < 2) || (argc > 3)) {
usage:
    printf("Usage:\n");
    printf(" %s <filename>\n", argv[0].str);
    printf(" %s debug\n", argv[0].str);
    printf(" %s run\n", argv[0].str);
    printf(" %s write <filename>\n", argv[0].str);
    return 0;
  }

  if (strcmp(argv[1].str, "debug") == 0) {
    Debug(4567);
    return 0;
  }

  if (strcmp(argv[1].str, "run") == 0) {
    FlashRun("qspi-flash");
    return 0;
  }

  const char* filename = 0;
  if (strcmp(argv[1].str, "write") == 0) {
    if (argc != 3)
      goto usage;
    fletch_mode = MODE_BURN;
    filename = argv[2].str;
  } else {
    filename = argv[1].str;
  }

  printf("mode: %s\n", fletch_mode == MODE_BURN ? "write" : "run");

  download_t* download = MakeDownload(filename);
  if (!download) {
    return -1;
  }

  tftp_set_write_client(download->name, &TftpCallback, download);
  printf("ready for %s over tftp (at %p)\n", download->name, download->start);
  return 0;
}

static void ServicesInit(const struct app_descriptor* app) {
  fletch_mode = MODE_RUN;
  tftp_server_init(NULL);
}

APP_START(frun)
  .init = ServicesInit,
  .entry = NULL,
  .flags = 0,
APP_END

STATIC_COMMAND_START
STATIC_COMMAND("fletch", "fletch vm via tftp", &FletchRunner)
STATIC_COMMAND_END(fletchrunner);

// vim: set expandtab ts=2 sw=2:

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
#include <kernel/thread.h>
#include <dev/display.h>

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

typedef struct {
  unsigned char* start;
  unsigned char* end;
  unsigned char* max;
  char name[FNAME_SIZE];
} download_t;

static download_t* make_download(const char* name, int slot) {
  download_t* d = malloc(sizeof(download_t));

  // use the page alloc api to grab space for the app
  d->start = page_alloc(DOWNLOAD_SLOT_SIZE / PAGE_SIZE);
  if (!d->start) {
    free(d);
    printf("error allocating slot for app\n");
    return NULL;
  }

  d->end = d->start;
  d->max = d->end + DOWNLOAD_SLOT_SIZE;

  strncpy(d->name, name, FNAME_SIZE);
  memset(d->start, 0, DOWNLOAD_SLOT_SIZE);

  return d;
}

static int run_snapshot(void * ctx) {
  download_t* d = ctx;

  printf("starting fletch-vm...\n");
  FletchSetup();

  int len = (d->end - d->start);
  printf("loading snapshot: %d bytes ...\n", len);
  FletchProgram program = FletchLoadSnapshot(d->start, len);

  printf("running program...\n");
  int result = FletchRunMain(program);

  printf("deleting program...\n");
  FletchDeleteProgram(program);

  printf("tearing down fletch-vm...\n");
  printf("vm exit code: %i\n", result);
  FletchTearDown();

  return result;
}

int tftp_callback(void* data, size_t len, void* arg) {
  download_t* download = arg;

  if (!data) {
    // Done with the download. Run the snapshot in a separate thread.
    thread_resume(thread_create(
      "fletch vm", &run_snapshot, download, DEFAULT_PRIORITY, 8192));

    // To reuse this slot : download->end = download->start;
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

static int fletch_runner(int argc, const cmd_args *argv) {
  // The 0th slot maps to the framebuffer so we start above that.
  static int slot = 1;

  if (argc != 2) {
    printf("fletch [filename]\n");
    return 0;
  }

  download_t* download = make_download(argv[1].str, slot);
  if (!download) {
    return -1;
  }

  tftp_set_write_client(download->name, &tftp_callback, download);
  printf("ready for %s over tftp (at %p)\n", download->name, download->start);
  slot++;
  return 0;
}

static void services_init(const struct app_descriptor *app) {
  tftp_server_init(NULL);
}

APP_START(network)
  .init = services_init,
  .entry = NULL,
  .flags = 0,
APP_END

STATIC_COMMAND_START
STATIC_COMMAND("fletch", "fletch vm via tftp", &fletch_runner)
STATIC_COMMAND_END(fletchrunner);

// vim: set expandtab ts=2 sw=2:

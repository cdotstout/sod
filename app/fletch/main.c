// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <app.h>
#include <err.h>
#include <compiler.h>
#include <endian.h>
#include <platform.h>
#include <dev/display.h>

#include <lib/font.h>
#include <lib/gfx.h>

#include <include/static_ffi.h>

#include "loader.h"

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>
#else
#error "fletch app needs a console"
#endif

enum {
  MODE_RUN,
  MODE_BURN
} fletch_mode;


//////////////// Dart FFI setup ///////////////////////////////////////////////

__WEAK status_t display_get_info(struct display_info *info) {
  return ERR_NOT_FOUND;
}

static gfx_surface* GetFullscreenSurface(void) {
  struct display_info info;
  if (display_get_info(&info) < 0)
    return NULL;
  return gfx_create_surface_from_display(&info);
}

static int GetWidth(gfx_surface* surface) {
  if (!surface)
    return 0;
  return surface->width;
}

static int GetHeight(gfx_surface* surface) {
  if (!surface)
    return 0;
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
    DebugSnapshot(4567);
    return 0;
  }

  if (strcmp(argv[1].str, "run") == 0) {
    LoadSnapshotFromFlash(NULL);
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

  printf("waiting for %s via TFTP. mode: %s\n",
         filename, 
         fletch_mode == MODE_BURN ? "write" : "run");

  if (fletch_mode == MODE_RUN) {
    LoadSnapshotFromNetwork(filename);
  } else {
    AddSnapshotToFlash(filename);
  }
  return 0;
}

static void ServicesInit(const struct app_descriptor* app) {
  fletch_mode = MODE_RUN;
  LoaderInit();
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

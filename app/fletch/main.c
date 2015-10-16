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
#include <fletch_api.h>
#include <endian.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <dev/display.h>

#include <lib/gfx.h>
#include <lib/tftp.h>

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>
#else
#error "fletch app needs a console"
#endif

#define DOWNLOAD_SLOT_SIZE (512 * 1024)

#if defined(SDRAM_BASE)
#define DOWNLOAD_BASE ((void*)(SDRAM_BASE))
#endif

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

#if WITH_KERNEL_VM
  status_t err = vmm_alloc(vmm_get_kernel_aspace(), "fletch app",
             DOWNLOAD_SLOT_SIZE, (void **)&d->start, 0, 0, 0);
  if (err < 0) {
    free(d);
    printf("error allocating slot for app\n");
    return NULL;
  }
#else
  d->start = DOWNLOAD_BASE + (DOWNLOAD_SLOT_SIZE * slot);
#endif
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

typedef struct {
  const char* const name;
  const void* const ptr;
} StaticFFISymbol;

static int FFITestMagicMeat(void) {
  return 0xbeef;
}

static int FFITestMagicVeg(void) {
  return 0x1eaf;
}

#if WITH_LIB_GFX
/*
 * Simple framebuffer stuff.
 */
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

#define LIB_GFX_EXPORTS 7
#else  // WITH_LIB_GFX
#define LIB_GFX_EXPORTS 0
#endif  // WITH_LIB_GFX

StaticFFISymbol table[] = { {"magic_meat", &FFITestMagicMeat},
                            {"magic_veg", &FFITestMagicVeg},
#if WITH_LIB_GFX
                            {"gfx_create", &GetFullscreenSurface},
                            {"gfx_width", &GetWidth},
                            {"gfx_height", &GetHeight},
                            {"gfx_destroy", &gfx_surface_destroy},
                            {"gfx_pixel", &gfx_putpixel},
                            {"gfx_clear", &gfx_clear},
                            {"gfx_flush", &gfx_flush},
#endif  // WITH_LIB_GFX
};

const void* const fletch_ffi_table_start = table;
const void* const fletch_ffi_table_end = table + 2 + LIB_GFX_EXPORTS;

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

// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
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

#include <kernel/port.h>

#include <lib/font.h>
#include <lib/gfx.h>

#include <include/static_ffi.h>
#include <include/dartino_api.h>

#include "loader.h"
#include "accelerometer.h"

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>
#else
#error "dartino app needs a console"
#endif

void SensorsInit(void);

enum {
  MODE_RUN,
  MODE_BURN
} dartino_mode;


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


DARTINO_EXPORT_TABLE_BEGIN
DARTINO_EXPORT_TABLE_ENTRY("font_draw_char", font_draw_char)
DARTINO_EXPORT_TABLE_ENTRY("gfx_create", GetFullscreenSurface)
DARTINO_EXPORT_TABLE_ENTRY("gfx_width", GetWidth)
DARTINO_EXPORT_TABLE_ENTRY("gfx_height", GetHeight)
DARTINO_EXPORT_TABLE_ENTRY("gfx_destroy", gfx_surface_destroy)
DARTINO_EXPORT_TABLE_ENTRY("gfx_pixel", gfx_putpixel)
DARTINO_EXPORT_TABLE_ENTRY("gfx_line", gfx_line)
DARTINO_EXPORT_TABLE_ENTRY("gfx_clear", gfx_clear)
DARTINO_EXPORT_TABLE_ENTRY("gfx_flush", gfx_flush)
DARTINO_EXPORT_TABLE_ENTRY("port_open", port_open)
DARTINO_EXPORT_TABLE_ENTRY("port_close", port_open)
DARTINO_EXPORT_TABLE_ENTRY("port_read", port_read)
DARTINO_EXPORT_TABLE_ENTRY("accelerometer_request_data",
                           accelerometer_request_data)
DARTINO_EXPORT_TABLE_END

//////////////// Port debug ///////////////////////////////////////////////////

static void DumpPacket(const port_result_t* result) {
  const port_packet_t* p = &result->packet;
  printf("[%02x %02x %02x %02x %02x %02x %02x %02x]\n",
         p->value[0], p->value[1], p->value[2], p->value[3],
         p->value[4], p->value[5], p->value[6], p->value[7]);
}

static void DumpPort(const char* name) {
  port_t port;
  status_t st = port_open(name, NULL, &port);
  if (st < 0) {
    printf("could not open port, error %d\n", st);
    return;
  }
  port_result_t result;
  while (true) {
    st = port_read(port, 0, &result);
    if (st < 0) {
      if (st == ERR_TIMED_OUT) {
        printf("done.\n");
      } else {
        printf("error %d reading packet\n", st);
      }
      break;
    }
    DumpPacket(&result);
  }
  port_close(port);
}

//////////////////// Starting/Stopping the VM ///////////////////////////////

static bool dartino_vm_was_started = false;

static void StartVM(void) {
  if (!dartino_vm_was_started) {
    printf("Starting dartino-vm...\n");
    DartinoSetup();
    dartino_vm_was_started = true;
  } else {
    printf("dartino-vm already running...\n");
  }
}

static void ShutdownVM(void) {
  if (dartino_vm_was_started) {
    printf("Stopping dartino-vm...\n");
    DartinoTearDown();
    dartino_vm_was_started = false;
  } else {
    printf("dartino-vm has not been started...\n");
  }
}

#define ENSURE_VM_IS_RUNNING(cmd)                                    \
if (!dartino_vm_was_started) {                                       \
  printf("Command %s requires a running VM.\n", cmd);                \
  return 0;                                                          \
}

//////////////// Shell handler ///////////////////////////////////////////////

static int DartinoRunner(int argc, const cmd_args* argv) {
  if ((argc < 2) || (argc > 3)) {
usage:
    printf("Usage:\n");
    printf(" %s start\n", argv[0].str);
    printf(" %s stop\n", argv[0].str);
    printf(" %s <filename>\n", argv[0].str);
    printf(" %s debug\n", argv[0].str);
    printf(" %s run <filename>\n", argv[0].str);
    printf(" %s install <filename>\n", argv[0].str);
    printf(" %s port_dump <name>\n", argv[0].str);
    return 0;
  }

  if (strcmp(argv[1].str, "start") == 0) {
    StartVM();
    return 0;
  }

  if (strcmp(argv[1].str, "stop") == 0) {
    ShutdownVM();
    return 0;
  }

  if (strcmp(argv[1].str, "debug") == 0) {
    ENSURE_VM_IS_RUNNING("debug");
    DebugSnapshot(4567);
    return 0;
  }

  if (strcmp(argv[1].str, "run") == 0) {
    ENSURE_VM_IS_RUNNING("run");
    LoadSnapshotFromFlash(argv[2].str);
    return 0;
  }

  if (strcmp(argv[1].str, "port_dump") == 0) {
    if (argc != 3)
      goto usage;
    DumpPort(argv[2].str);
    return 0;
  }

  const char* filename = 0;
  if (strcmp(argv[1].str, "install") == 0) {
    if (argc != 3)
      goto usage;
    dartino_mode = MODE_BURN;
    filename = argv[2].str;
  } else {
    ENSURE_VM_IS_RUNNING("");
    filename = argv[1].str;
  }

  printf("waiting for %s via TFTP. mode: %s\n",
         filename,
         dartino_mode == MODE_BURN ? "install" : "run");

  if (dartino_mode == MODE_RUN) {
    LoadSnapshotFromNetwork(filename);
  } else {
    AddSnapshotToFlash(filename);
  }
  return 0;
}

static void ServicesInit(const struct app_descriptor* app) {
  dartino_mode = MODE_RUN;
  SensorsInit();
  LoaderInit();
}

APP_START(frun)
  .init = ServicesInit,
  .entry = NULL,
  .flags = 0,
APP_END

STATIC_COMMAND_START
STATIC_COMMAND("dartino", "dartino vm via tftp", &DartinoRunner)
STATIC_COMMAND_END(dartinorunner);

// vim: set expandtab ts=2 sw=2:

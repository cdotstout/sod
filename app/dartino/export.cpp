// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include <err.h>

#include <include/static_ffi.h>

#include <dev/display.h>
extern "C" {
#include <lib/font.h>
#include <lib/gfx.h>
#include "accelerometer.h"
}
#include <kernel/port.h>

#include "graphics.h"

#define PAINT(x) reinterpret_cast<SkPaint*>(x)
#define PATH(x) reinterpret_cast<SkPath*>(x)
#define CANVAS(x) reinterpret_cast<SkCanvas*>(x)
#define IMAGE(x) reinterpret_cast<ImageWrapper*>(x)->image()
#define PIPE(x) reinterpret_cast<PipeWrapper*>(x)->pipe()

//////////////// Dart FFI setup ///////////////////////////////////////////////

__WEAK status_t display_get_framebuffer(struct display_framebuffer *fb) {
  return ERR_NOT_FOUND;
}

static gfx_surface* GetFullscreenSurface(void) {
  struct display_framebuffer fb;
  if (display_get_framebuffer(&fb) < 0)
    return NULL;
  return gfx_create_surface_from_display(&fb);
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

//////////////////////////////////////////////////////////////////////////////

static void* paint_new() {
  auto paint = new SkPaint();
  return paint;
}

static void paint_delete(void* paint) {
  delete PAINT(paint);
}

static void paint_reset(void* paint) {
  PAINT(paint)->reset();
}

static void paint_setStyle(void* paint, int style) {
  PAINT(paint)->setStyle(static_cast<SkPaint::Style>(style));
}

static void paint_setColor(void* paint, int color) {
  PAINT(paint)->setColor(color);
}

//////////////////////////////////////////////////////////////////////////////

static void* path_new() {
  auto path = new SkPath();
  return path;
}

static void path_delete(void* path) {
  delete PATH(path);
}

static void path_moveTo(void* path, float x, float y) {
  PATH(path)->moveTo(x, y);
}

static void path_lineTo(void* path, float x, float y) {
  PATH(path)->lineTo(x, y);
}

static void path_cubicTo(void* path, float x, float y, float x1, float y1, float x2, float y2) {
  PATH(path)->cubicTo(x, y, x1, y1, x2, y2);
}

static void path_close(void* path) {
  PATH(path)->close();
}

//////////////////////////////////////////////////////////////////////////////

static void* canvas_new(void* image) {
  auto canvas = new SkCanvas(IMAGE(image)->getBitmap());
  return canvas;
}

static void canvas_delete(void* canvas) {
  delete CANVAS(canvas);
}

static void canvas_resetMatrix(void* canvas) {
  CANVAS(canvas)->resetMatrix();
}

static void canvas_scale(void* canvas, float sx, float sy) {
  CANVAS(canvas)->scale(sx, sy);
}

static void canvas_translate(void* canvas, float tx, float ty) {
  CANVAS(canvas)->translate(tx, ty);
}

static void canvas_rotate(void* canvas, float degrees) {
  CANVAS(canvas)->rotate(degrees);
}

static void canvas_drawPath(void* canvas, void* path, void* paint) {
  CANVAS(canvas)->drawPath(*PATH(path), *PAINT(paint));
}

static void canvas_drawColor(void* canvas, int color) {
  CANVAS(canvas)->drawColor(color);
}

//////////////////////////////////////////////////////////////////////////////

static void* display_open() {
  return RcDisplay::open();
}

static void display_close(void* display) {
  if (display)
    reinterpret_cast<RcDisplay*>(display)->unref();
}

static void* display_createImage(void* display) {
  return ImageWrapper::create(reinterpret_cast<RcDisplay*>(display));
}

static void image_delete(void* image) {
  delete reinterpret_cast<ImageWrapper*>(image);
}

static int image_getWidth(void* image) {
  return IMAGE(image)->getBitmap().width();
}

static int image_getHeight(void* image) {
  return IMAGE(image)->getBitmap().width();
}

static void* display_createImagePipe(void* display, void* image0, void* image1, void* image2) {
  DisplayableImage* images[] { image0 ? IMAGE(image0) : nullptr,
    image1 ? IMAGE(image1) : nullptr,
    image2 ? IMAGE(image2) : nullptr
  };
  return PipeWrapper::create(reinterpret_cast<RcDisplay*>(display), images);
}

static void pipe_delete(void* pipe) {
  delete reinterpret_cast<PipeWrapper*>(pipe);
}

static int pipe_getMaxPipeDepth() {
  return DisplayablePipe::getMaxPipeDepth();
}

static int pipe_getCurrent(void* pipe) {
  return PIPE(pipe)->getCurrent();
}

static void pipe_present(void* pipe) {
  PIPE(pipe)->present();
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
DARTINO_EXPORT_TABLE_ENTRY("port_create", port_create)
DARTINO_EXPORT_TABLE_ENTRY("port_destroy", port_destroy)
DARTINO_EXPORT_TABLE_ENTRY("port_open", port_open)
DARTINO_EXPORT_TABLE_ENTRY("port_close", port_open)
DARTINO_EXPORT_TABLE_ENTRY("port_read", port_read)
DARTINO_EXPORT_TABLE_ENTRY("port_write", port_write)
DARTINO_EXPORT_TABLE_ENTRY("accelerometer_request_data",
                           accelerometer_request_data)

DARTINO_EXPORT_TABLE_ENTRY("paint_new", paint_new)
DARTINO_EXPORT_TABLE_ENTRY("paint_delete", paint_delete)
DARTINO_EXPORT_TABLE_ENTRY("paint_reset", paint_reset)
DARTINO_EXPORT_TABLE_ENTRY("paint_setStyle", paint_setStyle)
DARTINO_EXPORT_TABLE_ENTRY("paint_setColor", paint_setColor)

DARTINO_EXPORT_TABLE_ENTRY("path_new", path_new)
DARTINO_EXPORT_TABLE_ENTRY("path_delete", path_delete)
DARTINO_EXPORT_TABLE_ENTRY("path_moveTo", path_moveTo)
DARTINO_EXPORT_TABLE_ENTRY("path_lineTo", path_lineTo)
DARTINO_EXPORT_TABLE_ENTRY("path_cubicTo", path_cubicTo)
DARTINO_EXPORT_TABLE_ENTRY("path_close", path_close)

DARTINO_EXPORT_TABLE_ENTRY("canvas_new", canvas_new)
DARTINO_EXPORT_TABLE_ENTRY("canvas_delete", canvas_delete)
DARTINO_EXPORT_TABLE_ENTRY("canvas_resetMatrix", canvas_resetMatrix)
DARTINO_EXPORT_TABLE_ENTRY("canvas_scale", canvas_scale)
DARTINO_EXPORT_TABLE_ENTRY("canvas_translate", canvas_translate)
DARTINO_EXPORT_TABLE_ENTRY("canvas_rotate", canvas_rotate)
DARTINO_EXPORT_TABLE_ENTRY("canvas_drawPath", canvas_drawPath)
DARTINO_EXPORT_TABLE_ENTRY("canvas_drawColor", canvas_drawColor)

DARTINO_EXPORT_TABLE_ENTRY("display_open", display_open)
DARTINO_EXPORT_TABLE_ENTRY("display_close", display_close)
DARTINO_EXPORT_TABLE_ENTRY("display_createImage", display_createImage)
DARTINO_EXPORT_TABLE_ENTRY("display_createImagePipe", display_createImagePipe)
DARTINO_EXPORT_TABLE_ENTRY("image_delete", image_delete)
DARTINO_EXPORT_TABLE_ENTRY("image_getWidth", image_getWidth)
DARTINO_EXPORT_TABLE_ENTRY("image_getHeight", image_getHeight)
DARTINO_EXPORT_TABLE_ENTRY("pipe_delete", pipe_delete)
DARTINO_EXPORT_TABLE_ENTRY("pipe_present", pipe_present)
DARTINO_EXPORT_TABLE_ENTRY("pipe_getMaxPipeDepth", pipe_getMaxPipeDepth)
DARTINO_EXPORT_TABLE_ENTRY("pipe_getCurrent", pipe_getCurrent)
DARTINO_EXPORT_TABLE_ENTRY("pipe_present", pipe_present)


DARTINO_EXPORT_TABLE_END

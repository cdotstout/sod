// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include <displaym.h>
#include <SkPaint.h>
#include <SkCanvas.h>
#include <SkPath.h>

class RcDisplay : public SkRefCnt {
public:
  static RcDisplay* open() {
    auto display = DisplayManager::Open();
    if (!display)
      return nullptr;
    return new RcDisplay(display);
  }

  Display* display() const {
    return display_;
  }

  void unref() const {
    SkRefCnt::unref();
  }

private:
  RcDisplay(Display* display) : display_(display) {}

  virtual ~RcDisplay() override {
    DisplayManager::Close(display_);
    display_ = nullptr;
  }

  Display* display_;
};

class ImageWrapper {
public:
  static ImageWrapper* create(RcDisplay* display) {
    auto image = display->display()->CreateImage();
    if (!image)
      return nullptr;
    return new ImageWrapper(display, image);
  }

  ~ImageWrapper() {
    display_->display()->DestroyImage(image_);
    display_->unref();
  }

  DisplayableImage* image() const {
    return image_;
  }

private:
  ImageWrapper(RcDisplay* display, DisplayableImage* image)
    : display_(display), image_(image) {
      display_->ref();
  }

  RcDisplay* display_;
  DisplayableImage* image_;
};

class PipeWrapper {
public:
  static PipeWrapper* create(RcDisplay* display, DisplayableImage* images[]) {
    auto pipe = display->display()->CreatePipe(images);
    if (!pipe)
      return nullptr;
    return new PipeWrapper(display, pipe);
  }

  ~PipeWrapper() {
    display_->display()->DestroyPipe(pipe_);
    display_->unref();
  }

  DisplayablePipe* pipe() const {
    return pipe_;
  }

private:
  PipeWrapper(RcDisplay* display, DisplayablePipe* pipe)
    : display_(display), pipe_(pipe) {
      display_->ref();
  }

  RcDisplay* display_;
  DisplayablePipe* pipe_;
};

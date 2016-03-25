// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#ifndef __SERVICES_DISPLAY_DISPLAYABLE_H
#define __SERVICES_DISPLAY_DISPLAYABLE_H

#include <SkBitmap.h>
#include <assert.h>

// Something that can be displayed, either a command list (TODO) or an image.
class Displayable {
 public:
  virtual ~Displayable() = 0;
};

// A displayable that takes the form of a raster/bitmap image.
class DisplayableImage : public Displayable {
 public:
  DisplayableImage(SkBitmap::Config config,
                   void* pixels,
                   int width,
                   int height,
                   int rowbytes);
  ~DisplayableImage() override{};

  SkBitmap& getBitmap() { return bitmap_; }

 private:
  SkBitmap bitmap_;
};

// DisplayablePipe contains an array of Displayables to allow for
// overlapped frame rendering.
class DisplayablePipe {
 public:
  // Present the given displayables.
  DisplayablePipe(DisplayableImage* images[]);
  virtual ~DisplayablePipe() {}

  static int getMaxPipeDepth() { return kMaxPipeDepth; }

  // Returns the displayable currently slotted to be rendered into.
  int getCurrent() { return 0; }

  Displayable* getDisplayable(int index) {
    DEBUG_ASSERT(index < kMaxPipeDepth);
    return displayable_[index];
  }

  // Present the 'current displayable' and switch the current if the pipe
  // contains > 1 displayable.
  virtual bool present() = 0;

 private:
  static const int kMaxPipeDepth = 1;  // TODO: support triple buffering
  Displayable* displayable_[kMaxPipeDepth]{};
};

#endif

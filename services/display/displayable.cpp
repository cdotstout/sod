// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include <displayable.h>

Displayable::~Displayable() {
}

//////////////////////////////////////////////////////////////////////////////

DisplayableImage::DisplayableImage(SkBitmap::Config config,
                                   void* pixels,
                                   int width,
                                   int height,
                                   int rowbytes) {
  bitmap_.setConfig(config, width, height, rowbytes);
  bitmap_.setPixels(pixels);
}

//////////////////////////////////////////////////////////////////////////////

DisplayablePipe::DisplayablePipe(DisplayableImage* images[]) {
  displayable_[0] = images[0];
}

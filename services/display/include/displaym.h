// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#ifndef __DEV_DISPLAYM_H
#define __DEV_DISPLAYM_H

#include <displayable.h>

class Display {
 public:
  // Creates an image that is displayable via a pipe.
  virtual DisplayableImage* CreateImage() = 0;
  virtual void DestroyImage(DisplayableImage* image) = 0;

  // Creates a pipe that will present onto this display.
  virtual DisplayablePipe* CreatePipe(DisplayableImage* image[]) = 0;
  virtual void DestroyPipe(DisplayablePipe* displayable) = 0;
};

class DisplayManager {
 public:
  static Display* Open();
  static void Close(Display* display);
};

#endif

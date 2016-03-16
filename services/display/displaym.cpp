// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include <dev/display.h>
#include <displaym.h>
#include <stdlib.h>

class Image : public DisplayableImage {
 public:
  Image(enum image_format format,
        SkBitmap::Config config,
        void* pixels,
        int width,
        int height,
        int rowbytes);

  struct display_image* getImage() {
    return &image_;
  }

  void release() { free(image_.pixels); }

 private:
  struct display_image image_;
};

Image::Image(enum image_format format,
             SkBitmap::Config config,
             void* pixels,
             int width,
             int height,
             int rowbytes)
    : DisplayableImage(config, pixels, width, height, rowbytes) {
  image_.format = format;
  image_.pixels = pixels;
  image_.width = width;
  image_.height = height;
  image_.rowbytes = rowbytes;
}

//////////////////////////////////////////////////////////////////////////////

class ImagePipe : public DisplayablePipe {
 public:
  ImagePipe(DisplayableImage* images[]) : DisplayablePipe(images) {}

  bool present() override;
};

bool ImagePipe::present() {
  auto image = static_cast<Image*>(getCurrent());
  display_present(image->getImage(), 0, image->getImage()->height);
  return true;
}

//////////////////////////////////////////////////////////////////////////////

class LocalDisplay : public Display {
 public:
  LocalDisplay();

  DisplayableImage* CreateImage() override;
  void DestroyImage(DisplayableImage* image) override;

  // Creates a pipe that will present onto this display.
  DisplayablePipe* CreatePipe(DisplayableImage* image[]) override;
  void DestroyPipe(DisplayablePipe* displayable) override;

 private:
  struct display_info display_info_;
};

LocalDisplay::LocalDisplay() {
  display_get_info(&display_info_);
}

DisplayableImage* LocalDisplay::CreateImage() {
  SkBitmap::Config config;
  enum image_format format;
  int rowbytes;

  switch (display_info_.format) {
    case DISPLAY_FORMAT_RGB_111:
      config = SkBitmap::kRGB_111_Config;
      format = IMAGE_FORMAT_RGB_x111;
      rowbytes = (display_info_.width + 1) >> 1;
      break;
    case DISPLAY_FORMAT_MONO_1:
      config = SkBitmap::kBW_Config;
      format = IMAGE_FORMAT_MONO_1;
      rowbytes = (display_info_.width + 7) >> 3;
      break;
    default:
      DEBUG_ASSERT(false);
      return nullptr;
  }

  void* pixels = malloc(display_info_.height * rowbytes);
  DEBUG_ASSERT(pixels);

  Image* image = new Image(format, config, pixels, display_info_.width,
                           display_info_.height, rowbytes);
  DEBUG_ASSERT(image);

  return image;
}

void LocalDisplay::DestroyImage(DisplayableImage* image_base) {
  DEBUG_ASSERT(image_base);
  auto image = static_cast<Image*>(image_base);
  image->release();
  delete image;
}

DisplayablePipe* LocalDisplay::CreatePipe(DisplayableImage* images[]) {
  auto pipe = new ImagePipe(images);
  DEBUG_ASSERT(pipe);
  return pipe;
}

void LocalDisplay::DestroyPipe(DisplayablePipe* pipe) {
  DEBUG_ASSERT(pipe);
  delete pipe;
}

//////////////////////////////////////////////////////////////////////////////

static Display* g_display[1];

Display* DisplayManager::Open() {
  if (!g_display[0]) {
    g_display[0] = new LocalDisplay();
  }
  return g_display[0];
};

void DisplayManager::Close(Display* display) {
    if (g_display[0]) {
        delete g_display[0];
        g_display[0] = nullptr;
    }
}

/*
 * Copyright (c) 2016 Craig Stout cstout@chromium.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <platform.h>
#include <lib/console.h>  // for cmd_args
#include <kernel/thread.h>
#include <displaym.h>

#include <SkDevice.h>
#include <SkBitmap.h>
#include <SkCanvas.h>

#include <svg_test.h>

class LkSvgTest : public SvgTest {
 public:
  LkSvgTest(int argc, const cmd_args* argv);

  int test();

 private:
  static int thread_entry(void* arg);
  int test_main();

  int run_time_ms_ = 0;
};

LkSvgTest::LkSvgTest(int argc, const cmd_args* argv) {
  if (argc > 1) {
    run_time_ms_ = argv[1].i * 1000;
  }
}

class DisplayResources {
 public:
  DisplayResources() { display_ = DisplayManager::Open(); }

  ~DisplayResources() {
    free();
    delete[] images_;
    DisplayManager::Close(display_);
  }

  bool alloc() {
    free();

    if (!images_) {
      images_ = new DisplayableImage* [DisplayablePipe::getMaxPipeDepth()]{};
    }
    for (int i = 0; i < DisplayablePipe::getMaxPipeDepth(); i++) {
      images_[i] = display_->CreateImage();
      if (!images_[i]) {
        return false;
      }
    }

    pipe_ = display_->CreatePipe(images_);
    if (!pipe_) {
      return false;
    }

    printf("display resources allocated\n");
    return true;
  }

  void free() {
    if (images_) {
      for (int i = 0; i < DisplayablePipe::getMaxPipeDepth(); i++) {
        if (images_[i]) {
          display_->DestroyImage(images_[i]);
          images_[i] = nullptr;
        }
      }
      printf("display resources freed\n");
    }
    delete pipe_;
    pipe_ = nullptr;
  }

  DisplayableImage* getCurrentImage() {
    return static_cast<DisplayableImage*>(
      pipe_->getDisplayable(pipe_->getCurrent()));
  }

  void present() { pipe_->present(); }

 private:
  Display* display_{};
  DisplayablePipe* pipe_{};
  DisplayableImage** images_{};
};

int LkSvgTest::test_main() {
  NSVGimage* svg = nullptr;
  DisplayResources display_res;

  if (!display_res.alloc()) {
    printf("Failed to allocated display resources\n");
    return -1;
  }

  switch (display_res.getCurrentImage()->getBitmap().config()) {
    case SkBitmap::Config::kBW_Config:
      svg = parse_cube();
      break;
    case SkBitmap::Config::kRGB_111_Config:
      svg = parse_colors();
      break;
    default:
      printf("unhandled bitmap config\n");
      return -1;
  }

  if (!svg) {
    printf("Could not open SVG image.\n");
    return -1;
  }

  printf("rasterizing svg %d x %d\n", (int)svg->width, (int)svg->height);

  float rotate = 0;
  int total_draw_time_ms = 0;
  int total_update_time_ms = 0;

  for (int frame_count = 0;; frame_count++) {
    lk_time_t start_time, draw_time, update_time;

    auto image = display_res.getCurrentImage();
    SkCanvas canvas(image->getBitmap());

    int w = canvas.getDevice()->width();
    int h = canvas.getDevice()->height();

    float scaleh = static_cast<float>(h) / static_cast<int>(svg->height);
    float scalew = static_cast<float>(w) / static_cast<int>(svg->width);
    float scale = scaleh < scalew ? scaleh : scalew;

    if (rotate) {
      canvas.translate(w / 2, h / 2);
      canvas.rotate(rotate);
      canvas.translate(-w / 2, -h / 2);
    }

    if (scale != 1.0) {
      canvas.scale(scale, scale);
    }

    start_time = current_time();

    draw(&canvas, svg, false);
    draw_time = current_time() - start_time;

    total_draw_time_ms += draw_time;

    display_res.present();

    update_time = current_time() - draw_time - start_time;

    total_update_time_ms += update_time;

    if (total_draw_time_ms + total_update_time_ms > 1000) {
      int avg_draw_time_ms = total_draw_time_ms / frame_count;
      int avg_update_time_ms = total_update_time_ms / frame_count;
      int fps =
          frame_count * 1000 / (total_draw_time_ms + total_update_time_ms);
      printf("avg draw time %d ms avg update time %d ms (fps %d)\n",
             avg_draw_time_ms, avg_update_time_ms, fps);

      total_update_time_ms = total_draw_time_ms = 0;
      frame_count = 0;
    }

    if (run_time_ms_ > 0) {
      run_time_ms_ -= (draw_time + update_time);
      if (run_time_ms_ < 0) {
        break;
      }
    }

    rotate += 1;
    if (rotate > 360) {
      rotate -= 360;
    }
  }

  nsvgDelete(svg);

  return 0;
}

int LkSvgTest::thread_entry(void* arg) {
  auto obj = static_cast<LkSvgTest*>(arg);
  return obj->test_main();
}

int LkSvgTest::test() {
  thread_t* parse_thread;
  int retcode;

  parse_thread = thread_create("svg_test", LkSvgTest::thread_entry, this,
                               DEFAULT_PRIORITY, 4096);

  if (!parse_thread) {
    printf("svg_test: couldn't create thread\n");
    return -1;
  }

  thread_resume(parse_thread);
  thread_join(parse_thread, &retcode, INFINITE_TIME);

  return retcode;
}

int svg_test(int argc, const cmd_args* argv) {
  LkSvgTest test(argc, argv);
  return test.test();
}

STATIC_COMMAND_START
STATIC_COMMAND("svg_test", "svg rendering and animation test", &svg_test)
STATIC_COMMAND_END(svg_test_cmd);

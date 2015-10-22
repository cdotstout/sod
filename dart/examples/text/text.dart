// Copyright (c) 2015, the SoD project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.
//

import 'package:lk/framebuffer.dart';
import 'package:lk/font.dart';

main () {
  var surface = new FrameBuffer();
  var font = new Font();

  int sw = surface.width;
  int sh = surface.height;
  int fw = font.width;
  int fh = font.height;

  while (true) {
    int color = 0xFFFFFFF0;
    int x = 0;
    int y = 0;

    while (color > 0) {
      for (int ch = 32; ch < 127; ch++) {
        font.drawChar(ch, x, y, color);
        x = x + fw + 1;
        if (x > sw) {
          x = 0;
          y = y + fh + 1;
          if (y > sw) {
            y = 0;   
            break;
          }
        }
      }
      color = color - 1;
    }
    surface.clear();
  }
}

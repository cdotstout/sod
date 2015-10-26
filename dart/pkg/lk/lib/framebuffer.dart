// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'dart:fletch.ffi';
import 'dart:math';

class FrameBuffer {
  final ForeignPointer _surface;

  static ForeignFunction _getFrameBuffer = ForeignLibrary.main.lookup('gfx_create');
  static ForeignFunction _getWidth = ForeignLibrary.main.lookup('gfx_width');
  static ForeignFunction _getHeight = ForeignLibrary.main.lookup('gfx_height');
  static ForeignFunction _clear = ForeignLibrary.main.lookup('gfx_clear');
  static ForeignFunction _flush = ForeignLibrary.main.lookup('gfx_flush');
  static ForeignFunction _pixel = ForeignLibrary.main.lookup('gfx_pixel');
  static ForeignFunction _line = ForeignLibrary.main.lookup('gfx_line');


  int get width => _getWidth.icall$1(_surface);
  int get height => _getHeight.icall$1(_surface);

  FrameBuffer() : _surface = _getFrameBuffer.pcall$0();

  clear() => _clear.vcall$1(_surface);

  flush() => _flush.vcall$1(_surface);

  drawPixel(int x, int y, int color) => 
    _pixel.vcall$4(_surface, x, y, color);

  drawLine(int x1, int y1, int x2, int y2, int color) =>
    _line.vcall$6(_surface, x1, y1, x2, y2, color);

}

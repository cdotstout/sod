// Copyright (c) 2015, the SoD project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'dart:dartino.ffi';

class Font {
  final ForeignPointer _surface;

  static ForeignFunction _draw_font =
      ForeignLibrary.main.lookup('font_draw_char');

  static ForeignFunction _getFrameBuffer =
      ForeignLibrary.main.lookup('gfx_create');

  int get width => 6;
  int get height => 12;

  Font() : _surface = _getFrameBuffer.pcall$0();

  drawChar(int ch, int x, int y, int color) =>
    _draw_font.vcall$5(_surface, ch, x, y, color);

}


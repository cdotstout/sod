// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

part of ui;

enum PaintStyle {
  fill,
  stroke,
  strokeAndFill,
}

class Paint {
  Paint() : _this = _new.pcall$0() {
    _this.registerFinalizer(_delete, _this.address);
  }

  reset() {
    _reset.vcall$0();
  }

  setStyle(PaintStyle style) {
    _setStyle.pcall$1(style);
  }

  setColor(int color) {
    _setColor.vcall$2(_this, color);
  }

  final ForeignPointer _this;

  static ForeignFunction _new = ForeignLibrary.main.lookup('paint_new');
  static ForeignFunction _delete = ForeignLibrary.main.lookup('paint_delete');
  static ForeignFunction _reset = ForeignLibrary.main.lookup('paint_reset');
  static ForeignFunction _setStyle = ForeignLibrary.main.lookup('paint_setStyle');
  static ForeignFunction _setColor = ForeignLibrary.main.lookup('paint_setColor');
}

class Path {
  Path() : _this = _new.pcall$0() {
    _this.registerFinalizer(_delete, _this.address);
  }

  moveTo(double x, double y) {
    _moveTo.vcall$3(_this, x, y);
  }

  lineTo(double x, double y) {
     _lineTo.vcall$3(_this, x, y);
  }

  cubicTo(double x1, double y1, double x2, double y2, double x3, double y3) {
    _cubicTo.vcall$7(_this, x1, y1, x2, y2, x3, y3);
  }

  close() {
    _close.vcall$1(_this);
  }

  final ForeignPointer _this;

  static ForeignFunction _new = ForeignLibrary.main.lookup('path_new');
  static ForeignFunction _delete = ForeignLibrary.main.lookup('path_delete');
  static ForeignFunction _moveTo = ForeignLibrary.main.lookup('path_moveTo');
  static ForeignFunction _lineTo = ForeignLibrary.main.lookup('path_lineTo');
  static ForeignFunction _cubicTo = ForeignLibrary.main.lookup('path_cubicTo');
  static ForeignFunction _close = ForeignLibrary.main.lookup('path_close');
}

class Canvas {
  Canvas(Image image) : _this = _new.pcall$1(image._this) {
    _this.registerFinalizer(_delete, _this.address);
  }

  resetMatrix() {
    _resetMatrix.vcall$1(_this);
  }

  scale(double sx, double sy) {
    _scale.vcall$3(_this, sx, sy);
  }

  rotate(double degrees) {
    _rotate.vcall$2(_this, degrees);
  }

  translate(double tx, double ty) {
    _translate.vcall$3(_this, tx, ty);
  }

  drawPath(Path path, Paint paint) {
    _drawPath.vcall$3(_this, path._this, paint._this);
  }

  drawColor(int color) {
    _drawColor.vcall$2(_this, color);
  }

  final ForeignPointer _this;

  static ForeignFunction _new = ForeignLibrary.main.lookup('canvas_new');
  static ForeignFunction _delete = ForeignLibrary.main.lookup('canvas_delete');
  static ForeignFunction _resetMatrix = ForeignLibrary.main.lookup('canvas_resetMatrix');
  static ForeignFunction _scale = ForeignLibrary.main.lookup('canvas_scale');
  static ForeignFunction _translate = ForeignLibrary.main.lookup('canvas_translate');
  static ForeignFunction _rotate = ForeignLibrary.main.lookup('canvas_rotate');
  static ForeignFunction _drawPath = ForeignLibrary.main.lookup('canvas_drawPath');
  static ForeignFunction _drawColor = ForeignLibrary.main.lookup('canvas_drawColor');
}

class CanvasPool {
  CanvasPool() : map_ = new Map<Image, Canvas>();

  Canvas getCanvas(Image image) {
    if (map_[image] == null) {
      map_[image] = new Canvas(image);
    }
    return map_[image];
  }

  Map<Image, Canvas> map_;
}

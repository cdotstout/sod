// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'dart:dartino';
import 'dart:dartino.os';
import 'package:lk/accelerometer.dart';

main() {
  Accelerometer accelerometer = new Accelerometer();

  while(true) {
    PositionVector v = accelerometer.read();
    if (v == null) {
      continue;
    }
    print("x: ${v.x} y: ${v.y}, z: ${v.z}");
  }
}

// Copyright (c) 2015, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import "dart:dartino";
import "dart:async";

int count = 0;

onTimer(Timer timer) {
  count++;
  print("on timer $count");

  if (count == 10) {
    timer.cancel();
  }
}

void main() {
  new Timer.periodic(const Duration(seconds: 1), onTimer);
}

// Copyright (c) 2015, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'dart:dartino';
import 'dart:dartino.os';
import 'package:lk/ports.dart';

final String lk_sw = "sys/io/sw";

void ButtonHandler(ReadPort port) {
  while (true) {
    var value = port.read();
    print("pressed $value");
  }
}

main() {
  ReadPort port;

  try {
    port = new ReadPort(lk_sw);
  } on ArgumentError catch (error) {
    print("Argument Error: $error");
  } on StateError catch (error) {
    print("State Error: $error");
  }

  Fiber.fork(() => ButtonHandler(port));
}

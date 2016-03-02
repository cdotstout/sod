// Copyright (c) 2015, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'package:lk/ports.dart';

main () {
  var buttons_port = new ReadPortSync("sys/io/sw");

  print("looping ..");

  int n = 0;
  while(n != 10) {
    var packet = buttons_port.read();
    if (packet == null) {
      print("[$n] error reading packet.");
      break;
    }
    print("[$n] button: $packet");
  }

  buttons_port.close();
}

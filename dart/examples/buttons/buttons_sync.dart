// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'package:lk/ports.dart';

main () {
  var buttons_port = new LKReadPort("sys/io/sw");
  var packet = new PortPacket();

  print("looping ..");

  int n = 0;
  while(n != 10) {
    int status = buttons_port.read(packet, -1);
    if (status < 0) {
      print("[$n] error: $status");
      break;
    }
    var d = packet.data;
    print("[$n] button: $d");
  }

  buttons_port.close();
}

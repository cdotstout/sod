// Copyright (c) 2016, the SoD project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

library accelerometer;

import 'dart:dartino';
import 'dart:dartino.ffi';
import 'dart:dartino.os';
import 'package:lk/status.dart';

class PositionVector {
  double x;
  double y;
  double z;
}

class Accelerometer {

  final String _accelPortName = "sys/io/acc";

  static ForeignFunction _request_data =
      ForeignLibrary.main.lookup('accelerometer_request_data');

  Channel _channel;
  Port _port;

  Accelerometer() {
    _channel = new Channel();
    _port = new Port(_channel);

    eventHandler.registerPortForNextEvent(_accelPortName, _port, READ_EVENT);
  }

  PositionVector read() {
    int status = _request_data.icall$0();
    if (status != NO_ERROR) {
      return null;
    }

    // Read the first packet from the channel.
    var val = _channel.receive();
    if (val != NO_ERROR) {
      return null;
    }

    // Read and return the position vector result.
    PositionVector result = new PositionVector();
    result.x = _channel.receive();
    result.y = _channel.receive();
    result.z = _channel.receive();

    return result;
  }

}

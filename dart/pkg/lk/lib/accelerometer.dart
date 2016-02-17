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

  static final _accelPortName = "sys/io/accel"

  static ForeignFunction _request_data =
      ForeignLibrary.main.lookup('accelerometer_request_data');

  Channel _channel;
  Port _port;

  Accelerometer() {
    _channel = new Channel();
    _port = new Port();

    eventHandler.registerPortForNextEvent(_accelPortName, _port, READ_EVENT);
  }

  PositionVector read() {
    int status = _request_data.icall$0();
    if (status != NO_ERROR) {
      return null;
    }

    // Read the first packet from the channel.
    var val = channel.receive();
    if (val != NO_ERROR) {
      return null;
    }

    // Read and return the position vector result.
    PositionVector result;
    result.x = channel.receive();
    result.y = channel.receive();
    result.z = channel.receive();

    return result;
  }

}

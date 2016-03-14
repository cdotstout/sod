// Copyright (c) 2016, the SoD project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

library accelerometer;

import 'dart:dartino';
import 'dart:dartino.ffi';
import 'dart:dartino.os';
import 'package:lk/status.dart';
import "package:lk/ports.dart";

class PositionVector {
  double x;
  double y;
  double z;
}

class Accelerometer {

  static const _accelPortName = "sys/io/acc";

  static final ForeignFunction _request_data =
      ForeignLibrary.main.lookup('accelerometer_request_data');

  ReadPort _port;

  factory Accelerometer() {
    try {
      return new Accelerometer._();
    } catch(e) {
      return null;
    }
  }

  Accelerometer._() {
    _port = new ReadPort(_accelPortName);
  }

  PositionVector read() {
    int status = _request_data.icall$0();
    if (status != NO_ERROR) {
      return null;
    }

    // Read the first packet from the channel.
    Packet retval = _port.read();
    if (retval.intValue != NO_ERROR) {
      return null;
    }

    // Read and return the position vector result.
    PositionVector result = new PositionVector();
    result.x = _port.read().doubleValue;
    result.y = _port.read().doubleValue;
    result.z = _port.read().doubleValue;

    return result;
  }

}

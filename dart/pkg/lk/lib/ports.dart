// Copyright (c) 2015, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

library ports;

import 'dart:dartino.ffi';
import 'dart:typed_data';

class PortPacket {
  Uint8List data = new Uint8List(8);

  ForeignMemory getForeign() {
    var b = data.buffer;
    return b.getForeign();
  }
}

class LKReadPort {
  final int _port;

  static final ForeignFunction _open =
      ForeignLibrary.main.lookup('port_open');
  static final ForeignFunction _close =
      ForeignLibrary.main.lookup('port_close');
  static final ForeignFunction _read =
      ForeignLibrary.main.lookup('port_read');

  LKReadPort._internal(this._port);

  factory LKReadPort(String name) {
    ForeignMemory cName = new ForeignMemory.fromStringAsUTF8(name);
    ForeignMemory mPort = new ForeignMemory.allocated(4);
    int status = _open.icall$3(
        cName, ForeignPointer.NULL, mPort);
    cName.free();
    if (status < 0) {
      throw "could not create $name LK port";
    }
    final lkPort = new LKReadPort._internal(mPort.getInt32(0));
    mPort.free();
    return lkPort;
  }

  int close() {
    return _close.icall$1(_port);
  }

  int read(PortPacket packet, int timeout) {
    if (timeout < 0) {
      return _read.icall$3(_port, 0xffffffff, packet.getForeign());
    } else {
      return _read.icall$3(_port, timeout, packet.getForeign());      
    }
  }

}

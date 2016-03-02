// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

library ports;

import 'dart:dartino' as dartino;
import 'dart:dartino.ffi';
import 'dart:dartino.os';
import 'package:lk/status.dart' as err;

class Packet {
  final ForeignMemory _foreign = new ForeignMemory.allocatedFinalized(8);

  Packet.fromParts(int tag, int pointer) {
    _foreign.setUint32(0, pointer);
    _foreign.setUint32(4, tag);
  }

  Packet.fromBytes(List bytes) {
    if (bytes.length != 8) throw new ArgumentError(bytes);
    for (int cnt = 0; cnt < 8; cnt++) {
      _foreign.setUint8(cnt, bytes[cnt]);
    }
  }

  Packet.fromInt(int value) {
    _foreign.setUint64(0, value);
  }

  Packet.fromDouble(double value) {
    _foreign.setFloat64(0, value);
  }

  Packet.fromFloats(double upper, double lower) {
    _foreign.setFloat32(0, lower);
    _foreign.setFloat32(4, upper);
  }

  Packet._fromResponse(ForeignMemory memory) {
    // Skip the 32 bit for context.
    _foreign.setUint64(0, memory.getUint64(4));
  }

  int get tag => _foreign.getUint32(4);
  int get address => _foreign.getUint32(0);
  int get intValue => _foreign.getUint64(0);
  double get doubleValue => _foreign.getFloat64(0);
  double get upperFloatValue => _foreign.getFloat32(4);
  double get lowerFloatValue => _foreign.getFloat32(0);


  int operator[](int index) => _foreign.getUint8(index);

  ForeignMemory asForeign({finalized: true}) {
    var memory;
    if (finalized) {
      memory = new ForeignMemory.allocatedFinalized(8);
    } else {
      memory = new ForeignMemory.allocated(8);
    }
    memory.setUint64(0, _foreign.getUint64(0));
    return memory;
  }

  toString() {
    return "[${this[7]}:${this[6]}:${this[5]}:${this[4]}"
           ":${this[3]}:${this[2]}:${this[1]}:${this[0]}]";
  }
}

abstract class Port {
  String get name;

  static final ForeignFunction _open =
      ForeignLibrary.main.lookup('port_open');
  static final ForeignFunction _close =
      ForeignLibrary.main.lookup('port_close');
  static final ForeignFunction _read =
      ForeignLibrary.main.lookup('port_read');
  static final ForeignFunction _write =
      ForeignLibrary.main.lookup('port_write');
  static final ForeignFunction _create =
      ForeignLibrary.main.lookup('port_create');
  static final ForeignFunction _destroy =
      ForeignLibrary.main.lookup('port_destroy');

  static const int INFINITE = 0xFFFFFFFF;
}

class WritePort implements Port {
  final String name;
  final ForeignMemory _port;

  int get _portAsPointerValue => _port.getInt32(0);

  static const int BROADCAST = 0;
  static const int UNICAST = 1;
  static const int BIG_BUFFER = 2;

  WritePort._(this.name, this._port);

  factory WritePort(String name, {mode: BROADCAST, allowExisting: true}) {
    ForeignMemory cName = new ForeignMemory.fromStringAsUTF8(name);
    ForeignMemory mPort =
        new ForeignMemory.allocatedFinalized(Foreign.machineWordSize);
    int status = Port._create.icall$3(cName, mode, mPort);
    cName.free();
    if (status != err.NO_ERROR &&
        (!allowExisting || status != err.ERR_ALREADY_EXISTS)) {
      print("Cannot create port $name. Error $status.");
      return null;
    }
    return new WritePort._(name, mPort);
  }

  write(Packet p) {
    int status = Port._write.icall$3(_portAsPointerValue, p.asForeign(), 1);
    if (status != err.NO_ERROR) {
      print("Cannot write port $name. Error $status.");
    }
  }

  destroy() {
    Port._destroy.icall$1(_portAsPointerValue);
  }
}

class ReadPort implements Port {
  final dartino.Channel _channel;
  final String name;

  ReadPort._(this._channel, this.name);

  factory ReadPort(String name) {
    var channel = new dartino.Channel();
    var port = new dartino.Port(channel);
    eventHandler.registerPortForNextEvent(name, port, READ_EVENT);
    return new ReadPort._(channel, name);
  }

  // TODO(herhut): Re-register the port once one-shot semantics
  //               have been implemented.
  Packet read() => new Packet.fromInt(_channel.receive());
}

class ReadPortSync implements Port {
  final String name;
  final ForeignMemory _port;

  int get _portAsPointerValue => _port.getInt32(0);

  ReadPortSync._(this.name, this._port);

  factory ReadPortSync(String name) {
    ForeignMemory cName = new ForeignMemory.fromStringAsUTF8(name);
    ForeignMemory mPort =
      new ForeignMemory.allocatedFinalized(Foreign.machineWordSize);
    int status = Port._open.icall$3(cName, 0, mPort);
    cName.free();
    if (status != err.NO_ERROR) {
      print("Cannot open port $name. Error $status.");
      return null;
    }
    return new ReadPortSync._(name, mPort);
  }

  Packet read({int timeout: Port.INFINITE}) {
    // We need 1 word ctx and 2 words payload.
    var memory = new ForeignMemory.allocated(12);

    int status = Port._read.icall$3(_portAsPointerValue, timeout, memory);
    if (status != err.NO_ERROR) {
      print("Read on $name failed. Error $status.");
      memory.free();
      return null;
    }
    Packet result = new Packet._fromResponse(memory);
    memory.free();

    return result;
  }

  close() {
    Port._close.icall$1(_port);
  }
}

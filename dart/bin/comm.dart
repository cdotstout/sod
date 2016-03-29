// Copyright (c) 2016, the Dartino project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';
import 'dart:io';

/// A communication port for interacting with an attached device.
abstract class CommPort {
  static Duration defaultTimeout = new Duration(seconds: 3);

  /// Connect to and return a connection to the specified device.
  /// If a timeout is not specified, then a default timeout will be used.
  /// A connect may timeout if the user does not have the appropriate
  /// permissions to access the given port.
  /// Any other problem will result in an exception on the returned Future.
  static Future<CommPort> open(String portName, {Duration timeout}) async {
    timeout ??= defaultTimeout;
    RandomAccessFile ttyFile =
        await new File(portName).open(mode: FileMode.WRITE).timeout(timeout);
    var comm = new LkTtyCommPort(portName, ttyFile);

    // If establishing a connection times out, return null
    if (await comm.init(timeout)) return comm;
    comm.close();
    return null;
  }

  /// The name of the communication port (e.g. /dev/ttyACM0 or COM3).
  final String name;

  CommPort(this.name);

  /// Send a command over the communication port
  /// and return the text that was received.
  /// If a timeout is not specified, then a default timeout will be used.
  Future<String> sendCommand(String text, {Duration timeout});

  /// Echo text from the communication port for the specified time period.
  Future<Null> echo(Duration duration);

  /// Close the port and return a future that completes.
  Future close();
}

/// An implementation of [CommPort] specific to LK that uses /dev/tty*
/// to interact the the connected device.
class LkTtyCommPort extends CommPort {
  /// The connection to the device.
  final RandomAccessFile ttyFile;

  LkTtyCommPort(String name, this.ttyFile) : super(name);

  /// Initialize the connection and return `true` if successful, else `false`.
  Future<bool> init(Duration timeout) async {
    String result = await sendCommand('', timeout: timeout);
    return result != null;
  }

  @override
  Future<String> sendCommand(String text, {Duration timeout}) async {
    timeout ??= CommPort.defaultTimeout;

    // Send the command
    await ttyFile.writeString('$text\n').timeout(timeout);

    // Wait for a response
    bool newline = false;
    StringBuffer received = new StringBuffer();
    while (true) {
      int byte = await ttyFile.readByte().timeout(timeout);
      var ch = new String.fromCharCode(byte);
      received.write(ch);
      if (newline && ch == ']') {
        return received.toString();
      }
      newline = ch == '\n' || ch == '\r';
    }
  }

  @override
  Future<Null> echo(Duration duration) async {
    DateTime endTime = new DateTime.now().add(duration);
    StringBuffer received = new StringBuffer();
    var lastCh;
    while (true) {
      Duration timeout = endTime.difference(new DateTime.now());
      int byte = await ttyFile.readByte().timeout(timeout).catchError((e) {
        print(received.toString());
        return -1;
      }, test: (e) => e is TimeoutException);
      if (byte == -1) break;
      var ch = new String.fromCharCode(byte);
      if (ch == '\n' || ch == '\r') {
        if (lastCh == '\r' && ch == '\n') {
          // don't echo line again
        } {
          String line = received.toString();
          received.clear();
          print(line);
          if (line.startsWith('Exited with code')) break;
        }
      } else {
        received.write(ch);
      }
      lastCh = ch;
    }
  }

  @override
  Future close() {
    return ttyFile.close();
  }
}

// Copyright (c) 2016, the Dartino project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';
import 'dart:io';
import 'dart:math' show min;

const bool _debug = false;

/// The default timeout when interacting with tftp.
Duration _timeout = new Duration(seconds: 3);

main(List<String> args) async {
  if (args.length != 2) {
    print('Usage: tftp <address> </path/of/file/to/be/sent>');
    exit(1);
  }
  String address = args[0];
  String path = args[1];
  String name = path.split(Platform.pathSeparator).last;
  print('Sending $path to $address : $name');
  await new TftpClient(address).putBinary(path, name);
  print('Success');
}

/// A simple wrapper for tftp to push a binary to a connected device.
///
/// See [UDP Socket Programming with Dart](http://jamesslocum.com/post/77759061182)
/// See [THE TFTP PROTOCOL (REVISION 2)](https://tools.ietf.org/html/rfc1350)
/// See [LK tftp.c](https://github.com/littlekernel/lk/blob/09dbf477df18cd04b36a73c169cc41629627bb6e/lib/tftp/tftp.c)
class TftpClient {
  static final port = 69;

  static final opCodeWrite = 2;
  static final opCodeData = 3;
  static final opCodeAck = 4;
  static final opCodeError = 5;

  /// The IP address of the connected device.
  final InternetAddress address;

  TftpClient(String address) : this.address = _lookup(address);

  /// Use the TFTP protocal to send the given file to the remote device.
  Future<Null> putBinary(String filePath, String remoteFile) async {
    File file = new File(filePath);
    if (!await file.exists()) {
      throw new TftpException('File does not exist: $filePath');
    }
    List<int> binaryData = await file.readAsBytes();

    print('Opening datagram socket to ${address.address}');
    var socket = await RawDatagramSocket.bind(InternetAddress.ANY_IP_V4, 0);
    if (socket == null) {
      throw new TftpException('Failed to bind: ${address.address}');
    }

    print('Sending from ${socket.address.address}:${socket.port}');
    try {
      await new _TftpPut(socket, address, remoteFile, binaryData).perform();
    } catch (e) {
      try {
        socket.close();
      } catch (e) {
        // ignore exception
      }
      rethrow;
    }

    print('Closing socket');
    socket.close();
  }
}

class TftpException implements Exception {
  final String message;

  TftpException(this.message);

  String toString() {
    if (message == null) return "TftpException";
    return "TftpException: $message";
  }
}

/// Base class for communicating with a remote device.
abstract class _TftpConnection {
  /// A completer for the operation.
  final Completer _completer = new Completer();

  /// Socket for communicating with remote device.
  final RawDatagramSocket socket;

  /// The address of the remote device.
  final InternetAddress address;

  /// Port used when communicating with remote device.
  /// This will change once the server accepts the TFTP request.
  int port = TftpClient.port;

  /// The subscription for receiving socket data.
  StreamSubscription<RawSocketEvent> _subscription;

  /// A completer for waiting on an ACK from the remote device.
  Completer _ackReceived = new Completer();

  _TftpConnection(this.socket, this.address) {
    _subscription = socket.listen(processEvent);
  }

  /// Return a future that completes when the entire operation has finished.
  Future<Null> get opFuture => _completer.future;

  /// Called when the operation is has finished or is aborted.
  /// If [errorMessage] is omitted or `null`
  /// then the operation is considered to be successfull.
  void opComplete() {
    if (_debug) print('operation complete');
    if (!_completer.isCompleted) _completer.complete();
    if (!_ackReceived.isCompleted) _ackReceived.complete();
    cancel();
  }

  /// Stop listening and processing events from the remote device
  Future<Null> cancel() async {
    if (_subscription != null) {
      await _subscription.cancel();
      _subscription = null;
    }
  }

  /// Process an incomming socket event.
  void processEvent(RawSocketEvent event) {
    if (_debug) print('received event: $event');
    // Sanity check the event is expected... e.g. a RawSocketEvent.READ

    Datagram payload = socket.receive();
    if (payload == null) {
      if (_debug) print('Received: null');
      return;
    }

    List<int> data = payload.data;
    if (_debug) print('received: ${_packetForLog(data)}');
    if (data == null || data.length < 2) {
      throw new TftpException('Received invalid data: $data');
    }

    int blockNum = _ackBlockNum(data);
    if (blockNum != null) {
      _ackReceived.complete();
      processACK(payload.port, blockNum);
      return;
    }

    throw new TftpException(
        _checkError(data) ?? 'Received invalid data: $data');
  }

  /// Process an incomming socket ACK, where
  /// [payloadPort] is the port # received from the remote device, and
  /// [blockNum] is the block number in the ACK.
  void processACK(int payloadPort, int blockNum);

  /// Send the specified packet to the device and return the response.
  /// The response is either an int indicating an ACK block number,
  /// or an error message.
  void sendPacket(List<int> packet) {
    int count = socket.send(packet, address, port);
    if (_debug) print('sent: ${_packetForLog(packet)}');
    if (count != packet.length) {
      throw new TftpException('Expected ${packet.length} bytes to be sent,'
          ' but $count bytes were sent instead');
    }
    _ackReceived = new Completer();
    _ackReceived.future.timeout(_timeout);
  }
}

/// Internal class for sending binary data to the device.
class _TftpPut extends _TftpConnection {
  final String remoteFile;
  final List<int> binaryData;
  int expectedBlockNum = 0;

  _TftpPut(RawDatagramSocket socket, InternetAddress address, this.remoteFile,
      this.binaryData)
      : super(socket, address);

  /// Perform the operation.
  Future<Null> perform() {
    if (_debug) print('sending write request: $remoteFile');
    sendPacket(_binaryWriteRequest(remoteFile));
    return opFuture;
  }

  @override
  void processACK(int payloadPort, int blockNum) {
    if (blockNum != expectedBlockNum) {
      throw new TftpException(
          'Expected ACK block $expectedBlockNum, but found $blockNum');
    }
    ++expectedBlockNum;
    if (blockNum == 0) {
      if (_debug) print('write request succeeded, port $payloadPort');
      port = payloadPort;
      // fall through to send first block of data
    }
    int offset = blockNum * 512;
    if (offset <= binaryData.length) {
      sendPacket(_binaryData(blockNum + 1, binaryData, offset));
      return;
    }
    opComplete();
  }
}

/// Return the [InternetAddress] for the given address.
InternetAddress _lookup(address) {
  if (address is InternetAddress) return address;
  if (address is String) return new InternetAddress(address);
  throw new TftpException('Invalid internet address: $address');
}

/// Return a packet with binary data to be sent to the device.
/// DATA packet from https://tools.ietf.org/html/rfc1350
///
///      2 bytes     2 bytes      n bytes
///      ----------------------------------
///     | Opcode |   Block #  |   Data     |
///      ----------------------------------
///
/// Opcode = 3 and Data is from 0 to 512 bytes.
/// If Data is 512 bytes long, the block is not the last block of data.
/// If Data is from zero to 511 bytes long, it signals end of the transfer.
List<int> _binaryData(int blockNum, List<int> data, int offset) {
  List<int> content = [];
  content.add(0);
  content.add(TftpClient.opCodeData);
  content.add(blockNum >> 8 & 0xFF);
  content.add(blockNum & 0xFF);
  content.addAll(data.sublist(offset, min(offset + 512, data.length)));
  return content;
}

/// Return a packet requesting write binary data.
/// RRQ/WRQ packet from https://tools.ietf.org/html/rfc1350
///
///       2 bytes     string    1 byte     string   1 byte
///      ------------------------------------------------
///     | Opcode |  Filename  |   0  |    Mode    |   0  |
///      ------------------------------------------------
///
/// Opcode = 1 (RRQ)
/// Opcode = 2 (WRQ)
List<int> _binaryWriteRequest(String remoteFileName) {
  List<int> content = [];
  content.add(0);
  content.add(TftpClient.opCodeWrite);
  content.addAll(remoteFileName.codeUnits);
  content.add(0);
  content.addAll('binary'.codeUnits);
  content.add(0);
  return content;
}

/// If the data is an ACK response,
/// then return the ACK block number, otherwise return `null`.
/// ACK packet from https://tools.ietf.org/html/rfc1350
///
///      2 bytes     2 bytes
///      ---------------------
///     | Opcode |   Block #  |
///      ---------------------
///
/// Opcode = 4 (ACK)
int _ackBlockNum(List<int> data) {
  if (data.length != 4 || _opcode(data) != TftpClient.opCodeAck) return null;
  return (data[2] << 8) + data[3];
}

/// If the data is an ERROR response,
/// then return the error code and message as a string,
/// otherwise return `null`.
// ERROR packet from https://tools.ietf.org/html/rfc1350
///
///      2 bytes     2 bytes      string    1 byte
///      -----------------------------------------
///     | Opcode |  ErrorCode |   ErrMsg   |   0  |
///      -----------------------------------------
///
/// Opcode = 5 (ERROR)
String _checkError(List<int> data) {
  if (data.length < 5 || _opcode(data) != TftpClient.opCodeError) return null;
  int errCode = (data[2] << 8) + data[3];
  var end = data.length - 1;
  while (data[end] == 0) --end;
  String errMsg = new String.fromCharCodes(data, 4, end);
  return '$errCode :: $errMsg';
}

/// Return the opcode in the packet
int _opcode(List<int> packet) => (packet[0] << 8) + packet[1];

/// Return a string representing the given packet.
String _packetForLog(List<int> packet) {
  if (packet == null || packet.length <= 12) return packet.toString();
  var buf = new StringBuffer('bytes ');
  buf.write(packet.length);
  buf.write(' ');
  String text = packet.sublist(0, 12).toString();
  buf.write(text.substring(0, text.length - 1));
  buf.write(' ...');
  return buf.toString();
}

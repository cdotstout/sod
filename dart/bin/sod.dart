// Copyright (c) 2016, the Dartino project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// The goal of this 'sod' command line application is wrapper all the various
// command line utilities and sod activities in a single front end.
// For example 'sod analyze </path/to/app>'
//         and 'sod run <snap-file> on </dev/tty>'.
//
// This is very similar to what dartino provides and may be merged with
// or replace by that command line front end.

import 'dart:async';
import 'dart:io';

import 'comm.dart';
import 'tftp_client.dart';
//import 'package:stack_trace/stack_trace.dart';

main(List<String> args) {
  // Chain.capture(() => sod(args));
  sod(args).catchError((SodException e) {
    print(e.message);
    if (e.exception != null) print(e.exception);
    if (e.stackTrace != null) print(e.stackTrace);
    exit(-1);
  }, test: (e) => e is SodException);
}

Future<Null> sod(List<String> args) async {
  // Process arguments
  if (args.length != 4 ||
      args[0] != 'run' ||
      !args[1].endsWith('.snap') ||
      args[2] != 'on' ||
      !args[3].startsWith('/dev/tty')) {
    print('Unexpected arguments: $args');
    print('');
    print('Launch SOD app on a connected device');
    print('Usage: run <*.snap> on <path/to/tty>');
    exit(1);
  }
  var snapPath = args[1];
  var ttyPath = args[3];
  var snapName = snapPath.substring(snapPath.lastIndexOf('/') + 1);

  // Ping the device to see if it is connected
  String deviceIp = '192.168.0.98';
  print('Pinging $deviceIp to see if it is connected');
  Process process = await Process.start('ping', ['-c1', deviceIp]);
  int exitCode;
  try {
    exitCode = await process.exitCode.timeout(new Duration(seconds: 5));
  } on TimeoutException {
    exitCode = -1;
  }
  if (exitCode != 0) {
    throw new SodException('Failed to ping device at $deviceIp\n'
        'Try disconnecting and reconnecting or power cycling the device.');
  }

  // Connect to tty device
  print('Connecting to $ttyPath');
  CommPort comm;
  try {
    comm = await CommPort.open(ttyPath);
  } catch (e, s) {
    throw new SodException('Exception connecting to $ttyPath', e, s);
  }

  try {
    // Start dartino on the device
    print('Starting dartino VM');
    String response = await comm.sendCommand('dartino start');
    print(response);
    if (response.contains('already running')) {
      //TODO(danrubel) determine if application is still running
      // and terminate it before launching new application.
      // Revisit reboot command and get it working for this.
      throw new SodException(
          'Must power cycle device before launch... work in progress');
    }

    // Signal the device to receive the payload
    //   ] dartino lines.snap
    //   waiting for lines.snap via TFTP. mode: run
    //       --- send binary via tftp here ---
    //   ] starting dartino-vm...
    //   loading snapshot: 31651 bytes ...
    //   running program...
    print('Requesting device receive binary');
    response = await comm.sendCommand('dartino $snapName');
    if (!(response.contains('waiting for') && response.contains('via TFTP'))) {
      throw new SodException('Unexpected response from device:\n$response');
    }

    print('Sending binary to device using tftp $deviceIp');
    await new TftpClient(deviceIp).putBinary(snapPath, snapName);
    print('Deploy complete');

    // Display output from the application running on the device.
    // TODO(danrubel) echo output until the application terminates
    // rather than for 8 seconds.
    await comm.echo(new Duration(seconds: 8));
  } catch (e, s) {
    try {
      await comm.close();
    } catch (_) {
      // ignore exception on close
    }
    if (e is SodException) rethrow;
    throw new SodException('Exception communicating with $ttyPath', e, s);
  }
  await comm.close();
}

class SodException implements Exception {
  final String message;
  final exception;
  final StackTrace stackTrace;

  SodException(this.message, [this.exception, this.stackTrace]);

  String toString() {
    var buf = new StringBuffer('SodException');
    if (message != null) buf.write(': $message');
    if (exception != null) buf.write('\n$exception');
    if (stackTrace != null) buf.write('\n$stackTrace');
    return buf.toString();
  }
}

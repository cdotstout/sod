// Copyright (c) 2015, the SoD project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'dart:dartino';

const int MESSAGE_COUNT = 1000;

void channelResponder(Channel output) {
  Channel input = new Channel();
  output.send(input);
  int message;
  do {
    message = input.receive();
    output.send(message - 1);
  } while (message > 0);
}

void main() {
  var input = new Channel();
  Fiber.fork(() => channelResponder(input));
  var output = input.receive();

  int i = MESSAGE_COUNT;
  int prev;
  while (i > 0) {
    output.send(i);
    prev = i;
    i = input.receive();
    if (i + 1 != prev) {
      print("unexpected {i}");
      break;
    }
  }

  output.send(0);
}



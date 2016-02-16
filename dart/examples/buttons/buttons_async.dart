// Copyright (c) 2015, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

import 'dart:dartino';
import 'dart:dartino.os';

final String lk_sw = "sys/io/sw";

void ButtonHandler(Channel channel) {
  while (true) {
    var value = channel.receive();
    print("pressed $value");
  }
}

main() {
  Channel channel = new Channel();
  Port port = new Port(channel);

  try {
    eventHandler.registerPortForNextEvent(lk_sw, port, READ_EVENT);
  } on ArgumentError catch (error) {
    print("Argument Error: $error");
  } on StateError catch (error) {
    print("State Error: $error");
  }

  Fiber.fork(() => ButtonHandler(channel));
}

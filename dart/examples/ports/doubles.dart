import "dart:dartino";
import "dart:math";
import "package:lk/ports.dart";

final String uri = "/gen/dbl";

void readDoubles() {
  ReadPort port;
  do {
    try {
      print("Creating read port for $uri...");
      port = new ReadPort(uri);
    } catch (_) {
      print("Failed to spawn ReadPort...");
      Fiber.yield();
    }
  } while (port == null);

  print("Created ReadPort $uri...");

  while (true) {
    var packet = port.read();
    print("Received $packet, which is ${packet.doubleValue}...");
  }
}

void writeDoubles() {
  WritePort port = new WritePort(uri);
  print("Created WritePort $uri...");
  Random rnd = new Random();

  for (var i = 0; i < 10; i++) {
    var value = rnd.nextDouble();
    Packet packet = new Packet.fromDouble(value);
    port.write(packet);
    print("Write $packet, which is ${packet.doubleValue} aka $value...");
    Fiber.yield();
  }
  Packet packet = new Packet.fromDouble(0.0);
  port.write(packet);
}

main() {
  Fiber.fork(writeDoubles);
  Fiber.fork(readDoubles);
}

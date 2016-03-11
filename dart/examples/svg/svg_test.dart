// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.
//

import 'package:lk/ui.dart';
import 'vector_parser.dart';

// Define the variable xml
//import 'xml_flag.dart';
import 'xml_colors.dart';

const bool verbose = false;

int test() {
  if (verbose) print('getting a display');

  var display = DisplayManager.openDisplay();
  if (display == null) {
    print('Failed to get a display');
    return 1;
  }

  if (DisplayManager.openDisplay() != null) {
    print("Shouldn't be able to open display twice");
    return 1;
  }

  if (verbose) print("creating images");

  List<Image> imageList = new List<Image>();
  for (int i = 0; i < ImagePipe.maxPipeDepth; i++) {
    var image = display.createImage();
    if (image == null) {
      print("Failed to create image $i");
      return 1;
    }
    imageList.add(image);
  }

  if (verbose) print("creating a pipe");

  var pipe = display.createImagePipe(imageList);
  if (pipe == null) {
    print('Failed to create pipe');
    return 1;
  }

  if (verbose) print('parsing a vector');

  var vector = VectorParser.parse(xml);
  if (vector == null) {
    print('Failed to parse vector');
    return 1;
  }

  if (verbose) print("vector width ${vector.viewportWidth} height ${vector.viewportHeight}");

  if (verbose) print("creating a paint");

  var paint = new Paint();
  if (paint == null) {
    print('Failed to create paint');
    return 1;
  }

  var canvasPool = new CanvasPool();
  double rotate = 0.0;
  int frame_count = 600;

  for (int frame = 0; frame < frame_count; frame++) {
    Image currentImage = pipe.getImage(pipe.getCurrent());

    var canvas = canvasPool.getCanvas(currentImage);
    canvas.resetMatrix();

    const int color = 0xffffffff;
    canvas.drawColor(color);

    int height = currentImage.getHeight();
    int width = currentImage.getWidth();

    double scaleh = height / vector.viewportHeight;
    double scalew = width / vector.viewportWidth;
    double scale = scaleh < scalew ? scaleh : scalew;

    if (rotate != 0) {
      canvas.translate(width / 2, height / 2);
      canvas.rotate(rotate);
      canvas.translate(-width / 2, -height / 2);
    }

    if (scale != 1.0) {
      canvas.scale(scale, scale);
    }

    if (vector.groupList != null) {
      for (var group_node in vector.groupList) {
        if (group_node.pathList != null) {
          for (var node in group_node.pathList) {
            if (node.path != null) {
              paint.setColor(node.fillColor);
              canvas.drawPath(node.path, paint);
            }
          }
        }
      }
    }

    pipe.present();

    rotate += 1.0;
    if (rotate >= 360) {
      rotate -= 360;
    }
  }

  // Native resources for painting objects are released implicityl.
  // Explicit control over native resources for display objects is optional.
  if (verbose) print('releasing resources');

  for (var image in imageList) {
    image.release();
  }
  pipe.release();
  display.release();

  if (verbose) print('Test reacquiring display');

  // Test that we can reacquire the display immediately
  if (DisplayManager.openDisplay() == null) {
    print('Unable to re-acquire display');
    return 1;
  }

  return 0;
}

main() {
  int ret = test();
  print('test returned $ret');
}

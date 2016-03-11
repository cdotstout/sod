// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

part of ui;

class Image {
    Image._internal(ForeignPointer p) : _this = p {
        assert(_this.address != 0);
        _this.registerFinalizer(_delete, _this.address);
    }

    int getWidth() {
        assert(_this != null);
        return _getWidth.icall$1(_this);
    }

    int getHeight() {
        assert(_this != null);
        return _getHeight.icall$1(_this);
    }

    release() {
        assert(_this != null);
        _this.removeFinalizer(_delete);
        _delete.vcall$1(_this);
        _this = null;
    }

    ForeignPointer _this;

    static ForeignFunction _delete = ForeignLibrary.main.lookup('image_delete');
    static ForeignFunction _getWidth = ForeignLibrary.main.lookup('image_getWidth');
    static ForeignFunction _getHeight = ForeignLibrary.main.lookup('image_getHeight');
}

class ImagePipe {
    ImagePipe._internal(ForeignPointer p, List imageList)
        : _this = p, _imageList = imageList {
        assert(_this.address != 0);
        _this.registerFinalizer(_delete, _this.address);
    }

    int getCurrent() {
        assert(_this != null);
        return _getCurrent.icall$1(_this);
    }

    Image getImage(int index) {
        return _imageList[index];
    }

    present() {
        assert(_this != null);
        _present.vcall$1(_this);
    }

    release() {
        assert(_this != null);
        _this.removeFinalizer(_delete);
        _delete.vcall$1(_this);
        _this = null;
    }

    ForeignPointer _this;
    final List _imageList;

    static final int maxPipeDepth = _getMaxPipeDepth.icall$0();

    static ForeignFunction _delete = ForeignLibrary.main.lookup('pipe_delete');
    static ForeignFunction _getMaxPipeDepth = ForeignLibrary.main.lookup('pipe_getMaxPipeDepth');
    static ForeignFunction _getCurrent = ForeignLibrary.main.lookup('pipe_getCurrent');
    static ForeignFunction _present = ForeignLibrary.main.lookup('pipe_present');
}

class Display {
    Display._internal(ForeignPointer p) : _this = p {
        assert(_this.address != 0);
        _this.registerFinalizer(_close, _this.address);
    }

    // Returns null if the image cannot be created.
    Image createImage() {
        assert(_this != null);
        var p = _createImage.pcall$1(_this);
        if (p.address == 0)
            return null;
        return new Image._internal(p);
    }

    // Returns null if the image pipe cannot be created.
    ImagePipe createImagePipe(List imageList) {
        assert(_this != null);
        ForeignPointer i0 = imageList[0]._this;
        ForeignPointer i1 = ImagePipe.maxPipeDepth >= 2 ? imageList[1]._this : ForeignPointer.NULL;
        ForeignPointer i2 = ImagePipe.maxPipeDepth >= 3 ? imageList[2]._this : ForeignPointer.NULL;
        ForeignPointer pipe = _createImagePipe.pcall$4(_this, i0, i1, i2);
        if (pipe.address == 0)
            return null;
        return new ImagePipe._internal(pipe, imageList);
    }

    release() {
        assert(_this != null);
        _this.removeFinalizer(_close);
        _close.vcall$1(_this);
        _this = null;
    }

    ForeignPointer _this;

    static ForeignFunction _close = ForeignLibrary.main.lookup('display_close');
    static ForeignFunction _createImage = ForeignLibrary.main.lookup('display_createImage');
    static ForeignFunction _createImagePipe = ForeignLibrary.main.lookup('display_createImagePipe');
}

class DisplayManager {

    // Returns null if the display cannot be opened
    static Display openDisplay() {
        ForeignPointer p = _openDisplay.pcall$0();
        if (p.address == 0)
            return null;
        return new Display._internal(p);
    }

    static ForeignFunction _openDisplay = ForeignLibrary.main.lookup('display_open');
}

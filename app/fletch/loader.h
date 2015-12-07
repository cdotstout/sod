// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#ifdef __cplusplus
extern "C" {
#endif


void LoaderInit(void);

int LoadSnapshotFromNetwork(const char* name);

int LoadSnapshotFromFlash(const char* name);

int AddSnapshotToFlash(const char* name);

int DebugSnapshot(int port);

#ifdef __cplusplus
}
#endif

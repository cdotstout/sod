// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void GenerateFlashtoolArgs(char* buffer, size_t len, void* address,
                           const char* file);

#ifdef __cplusplus
}
#endif

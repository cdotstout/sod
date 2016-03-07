// Copyright (c) 2016, the Dartino project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include "generate_flashtool_args.h"

#include <stdio.h>

#include "src/vm/intrinsics.h"

// This symbol is defined by the dartino VM.
extern "C" void InterpreterMethodEntry(void);

void GenerateFlashtoolArgs(char* buffer, size_t len, void* address,
                           const char* file) {
  int pos = 0;
#define PRINT_INTRINSIC(_name) \
  pos += snprintf(buffer + pos, len - pos, "-i " #_name "=%p ", &dartino::Intrinsic_##_name);
  INTRINSICS_DO(PRINT_INTRINSIC)
#undef PRINT_INTRINSIC
  pos += snprintf(buffer + pos, len - pos, "%p <snapshot file> %p %s",
                  &InterpreterMethodEntry, address, file);
}


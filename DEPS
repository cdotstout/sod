# Copyright (c) 2015, the SoD project authors. Please see the AUTHORS file
# for details. All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE.md file.
#
# This file is automatically processed to create .DEPS.git which is the file
# that gclient uses under git.
#
# See http://code.google.com/p/chromium/wiki/UsingGit
#
# To test manually, run:
#   python tools/deps2git/deps2git.py -o .DEPS.git -w <gclientdir>
# where <gcliendir> is the absolute path to the directory containing the
# .gclient file.
#
# Then commit .DEPS.git locally (gclient doesn't like dirty trees) and run
#   gclient sync
# Verify the thing happened you wanted. Then revert your .DEPS.git change
# DO NOT CHECK IN CHANGES TO .DEPS.git upstream. It will be automatically
# updated by a bot when you modify this one.
#
# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.

vars = {
  "github_url": "https://github.com/%s.git",

  "github_mirror":
      "https://chromium.googlesource.com/external/github.com/dart-lang/%s.git",

  "chromium_git": "https://chromium.googlesource.com",

  "lk_rev": "@4895ead73951c1a75c4587e8082926251b3cfebf",

  # Fletch repo and dependencies.
  "fletch_rev": "@f7ed30aa421c554edca8d015fa0d8de4eb51c040",
  "gyp_rev": "@6ee91ad8659871916f9aa840d42e1513befdf638",
  "dart_rev": "@3210196ad25894f9cce82f0f0fc0044c3ef8d5b1",
  "persistent_rev": "@55daae1a038188c49e36a64e7ef132c4861da3d8",
  "charcode_tag": "@1.1.0",
  "path_tag": "@1.3.6",
  "package_config_tag": "@0.1.3",
}

deps = {
  "sod/third_party/lk":
      (Var("github_url") % "travisg/lk") + Var("lk_rev"),

  # Fletch repo and dependencies.
  "sod/third_party/fletch":
      (Var("github_url") % "dart-lang/fletch") + Var("fletch_rev"),

  "sod/third_party/fletch/third_party/gyp":
      Var('chromium_git') + '/external/gyp.git' + Var("gyp_rev"),

  "sod/third_party/fletch/third_party/dart":
      (Var("github_mirror") % "sdk") + Var("dart_rev"),

  "sod/third_party/fletch/third_party/persistent":
      (Var("github_url") % "polux/persistent") + Var("persistent_rev"),

  "sod/third_party/fletch/third_party/charcode":
      (Var("github_mirror") % "charcode") + Var("charcode_tag"),

  "sod/third_party/fletch/third_party/path":
      (Var("github_mirror") % "path") + Var("path_tag"),

  "sod/third_party/fletch/third_party/package_config":
      (Var("github_mirror") % "package_config") + Var("package_config_tag"),
}

hooks = [
  {
    'name': 'fletch_third_party_libs',
    'pattern': '.',
    'action': [
      'download_from_google_storage',
      '--no_auth',
      '--no_resume',
      '--bucket',
      'dart-dependencies-fletch',
      '-d',
      '-r',
      '--auto_platform',
      'sod/third_party/fletch/third_party/libs',
    ],
  },
  {
    'name': 'fletch_third_party_binaries',
    'pattern': '.',
    'action': [
      'download_from_google_storage',
      '--no_auth',
      '--no_resume',
      '--bucket',
      'dart-dependencies-fletch',
      '-d',
      '-r',
      '--auto_platform',
      'sod/third_party/fletch/third_party/bin',
    ],
  },
]

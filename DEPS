# Copyright (c) 2015, the SoD project authors. Please see the AUTHORS file
# for details. All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE.md file.
#
# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.

vars = {
  "github_url": "https://github.com/%s.git",

  "github_mirror":
      "https://chromium.googlesource.com/external/github.com/dart-lang/%s.git",

  "chromium_git": "https://chromium.googlesource.com",

  "lk_rev": "@b822b1f64f4a98ad10c9794e8ed20ab8ccba3d4a",

  # Fletch repo and dependencies.
  "fletch_rev": "@5e610881a2ff146bf0039b00ed4d5a89911dcb82",
  "gyp_rev": "@6ee91ad8659871916f9aa840d42e1513befdf638",
  "dart_rev": "@81f7629dc13c232833d181c908a869f01422dd05",
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
  {
    'name': 'third_party_openocd',
    'pattern': '.',
    'action': [
      'download_from_google_storage',
      '--no_auth',
      '--no_resume',
      '--bucket',
      'dart-dependencies-fletch',
      '-d',
      '-r',
      '-u',
      '--auto_platform',
      'sod/third_party/openocd',
    ],
  },
]

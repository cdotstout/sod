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

  "lk_rev": "@6b208387f5e44fdf19b47dd49cea5c153aadc270",

  # Dartino repo and dependencies.
  "dartino_rev": "@f4e73d8b83fff70995014731674b339492013271",
  "gyp_rev": "@2b086540aeedaf4d233766fd14fb674af1bc321d",
  "dart_rev": "@a5163f3595ccc4f41147a552c31c33090fe3970b",
  "persistent_rev": "@55daae1a038188c49e36a64e7ef132c4861da3d8",
  "charcode_tag": "@1.1.0",
  "path_tag": "@1.3.6",
  "package_config_tag": "@0.1.3",
  "collection_rev": "@1da9a07f32efa2ba0c391b289e2037391e31da0e",
  "dart2js_info_rev" : "@0a221eaf16aec3879c45719de656680ccb80d8a1",
  "pub_semver_tag": "@1.2.1",
}

deps = {
  "sod/third_party/lk":
      (Var("github_url") % "littlekernel/lk") + Var("lk_rev"),

  # Fletch repo and dependencies.
  "sod/third_party/fletch":
      (Var("github_url") % "dartino/sdk") + Var("dartino_rev"),

  "sod/third_party/fletch/third_party/gyp":
      Var('chromium_git') + '/external/gyp.git' + Var("gyp_rev"),

  "sod/third_party/fletch/third_party/dart":
      (Var("github_mirror") % "sdk") + Var("dart_rev"),

  "fletch/third_party/pub_semver":
      (Var("github_mirror") % "pub_semver") + Var("pub_semver_tag"),

  "sod/third_party/fletch/third_party/persistent":
      (Var("github_url") % "polux/persistent") + Var("persistent_rev"),

  "sod/third_party/fletch/third_party/charcode":
      (Var("github_mirror") % "charcode") + Var("charcode_tag"),

  "sod/third_party/fletch/third_party/path":
      (Var("github_mirror") % "path") + Var("path_tag"),

  "sod/third_party/fletch/third_party/package_config":
      (Var("github_mirror") % "package_config") + Var("package_config_tag"),

  "sod/third_party/fletch/third_party/collection":
      (Var("github_mirror") % "collection") + Var("collection_rev"),

  "sod/third_party/fletch/third_party/dart2js_info":
      "https://github.com/dart-lang/dart2js_info.git" + Var("dart2js_info_rev"),

  "sod/third_party/fletch/third_party/pub_semver":
      (Var("github_mirror") % "pub_semver") + Var("pub_semver_tag"),
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

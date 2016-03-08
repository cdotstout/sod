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

  "lk_rev": "@87fb7721f5494f1042e5a33856c2561f52bc9f62",
  "gbskia_rev": "@0926c6539d16c434bd21e149992ed0646fd465ad",

  # Dartino repo and dependencies.
  "dartino_rev": "@5cbae0ea6e4189ed7ea94684470643355cbab077",
  "gyp_rev": "@6fb8bd829f0ca8fd432fd85ede788b6881c4f09f",
  # This has to be in sync with the version used by dartino_rev above, as the
  # dartino git repo contains sha1 files to download dart vm binaries from
  # cloud storage.
  "dart_rev": "@6eed25f3142039cdc92097c4db27c6cb312581d8",
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

  "sod/third_party/lk/external/gbskia":
      (Var("github_url") % "littlekernel/gbskia") + Var("gbskia_rev"),

  # Dartino repo and dependencies.
  "sod/third_party/dartino":
      (Var("github_url") % "dartino/sdk") + Var("dartino_rev"),

  "sod/third_party/dartino/third_party/gyp":
      Var('chromium_git') + '/external/gyp.git' + Var("gyp_rev"),

  "sod/third_party/dartino/third_party/dart":
      (Var("github_mirror") % "sdk") + Var("dart_rev"),

  "sod/third_party/dartino/third_party/persistent":
      (Var("github_url") % "polux/persistent") + Var("persistent_rev"),

  "sod/third_party/dartino/third_party/charcode":
      (Var("github_mirror") % "charcode") + Var("charcode_tag"),

  "sod/third_party/dartino/third_party/path":
      (Var("github_mirror") % "path") + Var("path_tag"),

  "sod/third_party/dartino/third_party/package_config":
      (Var("github_mirror") % "package_config") + Var("package_config_tag"),

  "sod/third_party/dartino/third_party/collection":
      (Var("github_mirror") % "collection") + Var("collection_rev"),

  "sod/third_party/dartino/third_party/dart2js_info":
      "https://github.com/dart-lang/dart2js_info.git" + Var("dart2js_info_rev"),

  "sod/third_party/dartino/third_party/pub_semver":
      (Var("github_mirror") % "pub_semver") + Var("pub_semver_tag"),
}

hooks = [
  {
    'name': 'dartino_third_party_binaries',
    'pattern': '.',
    'action': [
      'download_from_google_storage',
      '--no_auth',
      '--no_resume',
      '--bucket',
      'dartino-dependencies',
      '-d',
      '-r',
      '--auto_platform',
      'sod/third_party/dartino/third_party/bin',
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
      'dartino-dependencies',
      '-d',
      '-r',
      '-u',
      '--auto_platform',
      'sod/third_party/openocd',
    ],
  },
]

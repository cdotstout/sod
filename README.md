# SoD
Seeds of Decay (or Sky on Devices).

# Building

## Prerequisites

* Install [depot_tools](http://www.chromium.org/developers/how-tos/install-depot-tools) (to get `gclient`, `git cl`, and `ninja`).

## Getting the Sources

    mkdir src   # (name doesn't matter)
    cd src
    create a file named .gclient with the contents below [1]
    gclient sync
    cd sod

[1] .gclient file:

    solutions = [
      { "name"        : "sod",
        "url"         : "https://github.com/domokit/sod.git",
        "deps_file"   : "DEPS",
        "managed"     : False,
        "custom_deps" : {
        },
        "safesync_url": "",
      },
    ]
    cache_dir = None


The above uses https. You can use ssh auth instead by changing `url` above for `"git@github.com:domokit/sod.git"`


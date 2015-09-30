# SoD

# Building

## Prerequisites

* Install [depot_tools](http://www.chromium.org/developers/how-tos/install-depot-tools) (to get `gclient`, `git cl`, and `ninja`).

## Getting the Sources

    mkdir src   # (name doesn't matter)
    cd src
    gclient config git@github.com:domokit/sod.git
    gclient sync
    cd sod

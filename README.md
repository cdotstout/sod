# SoD
Seeds of Decay (or Sky on Devices).

# Building

## Prerequisites

* Install [depot_tools](http://www.chromium.org/developers/how-tos/install-depot-tools) (to get `gclient`, `git cl`, and `ninja`).

## Getting the Sources

    mkdir src   # (name doesn't matter)
    cd src
    gclient config --unmanaged https://github.com/domokit/sod.git
    gclient sync
    cd sod

Note that after gclient sync finishes there is no branch setup (aka
detached head). You should then create a branch (don't name it master)
and start working there. For example:

    sod$ git new-branch foobar
    Switched to branch foobar.

This sets up the branch to track origin/master.

The above uses https. You can use ssh auth instead by changing
`"https://github.com/domokit/sod.git"` above for
`"git@github.com:domokit/sod.git"`


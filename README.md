# SoD
Seeds of Decay (or Sky on Devices). A software stack for running
[Dart](https://www.dartlang.org/) on small devices. It combines
[LK](https://github.com/travisg/lk) and [Fletch](https://github.com/dart-lang/fletch).


## Prerequisites
* arm-none-eabi-gcc 4.8 or better
* qemu-system-arm 2.4.0.1 or better
* [chromium depot_tools](http://www.chromium.org/developers/how-tos/install-depot-tools)
(to get `gclient`, `git cl`, and `ninja`).

## Getting the Sources

    mkdir src   # (name doesn't matter)
    cd src
    gclient config --unmanaged https://github.com/domokit/sod.git
    gclient sync
    cd sod

Note that after gclient sync finishes there is no git branch setup (aka
detached head). You should then create a branch (don't name it master)
and start working there. For example:

    sod$ git new-branch foobar

This sets up the branch to track origin/master.

The above uses https. You can use ssh auth instead by changing
`"https://github.com/domokit/sod.git"` above for
`"git@github.com:domokit/sod.git"`

## Building
The main binary is \out\build-$target\lk.bin for flashing into dev boards or
lk.elf for qemu runs.

Build all the targets:

    sod$ make all
    
Build for the default target (currently stm32f746g-disco-fletch):

    sod$ make lk

Create a fletch snapshot given lines.dart:

    sod$ make third_party/fletch/samples/lk/gfx/lines.snap

If this command fails because it can't find lk packages, you probably need to
edit `~/local.fletch-settings` and be sure to `fletch shutdown`.


Inspect the root makefile for other targets.

### Running
Currently the STM32F7 [disco](http://www.st.com/web/catalog/tools/FM116/CL1620/SC959/SS1532/LN1848/PF261641)
board and qemu arm are supported.

Build and run in qemu:

    sod$ make qemu-run

This runs qemu with a window that simulates a VGA framebuffer and an interative
LK shell redirected to the terminal. The prompt symbol is ']'.

To run dart code you need to create a snapshot and send it over the network
to the board or qemu instance.

    ] fletch lines.snap
    
Will wait for a dart snapshot (named lines.snap) sent via tftp from the host.
Note that in qemu the tftp port is redirected to 10069.

Assuming qemu, on linux:

    sod$ cd third_party/fletch/samples/lk/gfx
    sod$ tftp 0.0.0.0 10069
    tftp> binary
    tftp> put lines.snap

After the transfer finishes the program will start running automatically. In
some cases you might need to substitute 0.0.0.0 with localhost.

The LK shell has other commands. To see the list of available shell commands
type "help". For example:

    ] threads

Displays basic info about all running threads.



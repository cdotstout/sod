# SoD
Seeds of Decay (or Sky on Devices). A software stack for running
[Dart](https://www.dartlang.org/) on small devices. It  combines
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

The above uses https. You can use ssh auth instead by changing
`"https://github.com/domokit/sod.git"` above for
`"git@github.com:domokit/sod.git"`

## Quick start with QEMU

Assuming that you have qemu and you cd into sod directory:

    sod$ make qemu-run
    
You should see no errors and "generating image: out/build-qemu-virt-fletch/lk.bin" as one of the last lines in the output. Then you should see the LK console starting, with "welcome to lk/MP". Then press enter and at the ']' prompt type:

    ] fletch lines.snap

Then you should see "waiting for lines.snap via TFTP. mode: run".  Open another console, cd into the sod directory:

    sod$ make dart/examples/lines/lines.snap

When that completes, in that console transfer the .snap file via tftp. Note that qemu has mapped the tftp port to port 10069:

    sod$ cd dart/examples/lines
    lines$ tftp 0.0.0.0 10069
    tftp> binary
    tftp> put lines.snap
    Sent 31609 bytes in 0.0 seconds

Then you should see lines being drawn in the QEMU VGA framebuffer.

Note that in some cases you might need to substitute 0.0.0.0 in the tftp command with 'localhost'.

## Building
The main binary is \out\build-$target\lk.bin for flashing into dev boards or
lk.elf for qemu runs.

Inspect the root makefile for other targets. For example the STM32F7 [disco](http://www.st.com/web/catalog/tools/FM116/CL1620/SC959/SS1532/LN1848/PF261641) is supported.

## LK

The LK shell has other commands; most of them are available in the shell even when dart code is executing. To see the list of available shell commands type "help". For example:

    ] threads

Displays basic info about all running threads.

## Making Changes

Note that after gclient sync finishes there is no git branch setup (aka detached head). You should then create a branch (don't name it master) and start working there. For example:

    sod$ git new-branch foobar

This sets up the branch to track origin/master.

The easiest way to propose changes to this project is using the Chromium workflow, which works best with branch per change.

 1. git new-branch <feature-name>
 2. make changes
 3. git commit         # to local repo
 4. git cl upload
 5. more changes       # repeat steps 3 and 4
 6. after 'lgtm' has been received
 7. git cl land

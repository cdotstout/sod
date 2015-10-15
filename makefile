# add to this rule any subrules that should be built by default
all: lk qemu

# simple rule to build the default lk binary
lk:
	$(MAKE) -f third_party/lk/makefile $(filter-out $@,$(MAKECMDGOALS))

# build lk for qemu
qemu:
	$(MAKE) -f third_party/lk/makefile qemu-virt-fletch

# build and run lk for qemu with a display
qemu-run: qemu
	qemu-system-arm -machine virt -cpu cortex-a15 \
		-m 8 -kernel out/build-qemu-virt-fletch/lk.elf \
		-device virtio-gpu-device -serial stdio \
		-netdev user,id=vmnic,hostname=qemu -device virtio-net-device,netdev=vmnic \
		-redir udp:10069::69

fletch-tool:
	ninja -C third_party/fletch/ && ninja -C third_party/fletch/out/ReleaseIA32

%.snap: %.dart fletch-tool
	third_party/fletch/out/ReleaseIA32/fletch export $< to $@

clean:
	rm -rf out

.PHONY: all lk clean qemu qemu-run fletch-tool

# vim: set noexpandtab:

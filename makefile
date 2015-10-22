# add to this rule any subrules that should be built by default
all: lk qemu

# simple rule to build the default lk binary
lk:
	$(MAKE) -f third_party/lk/makefile $(filter-out $@,$(MAKECMDGOALS))

# build lk for qemu
qemu:
	$(MAKE) -f third_party/lk/makefile qemu-virt-fletch

# build lk for the disco dev boards
disco:
	$(MAKE) -f third_party/lk/makefile stm32f746g-disco-fletch

# build and run lk for qemu with a display
qemu-run: qemu
	qemu-system-arm -machine virt -cpu cortex-a15 \
		-m 12 -kernel out/build-qemu-virt-fletch/lk.elf \
		-device virtio-gpu-device -serial stdio \
		-netdev user,id=vmnic,hostname=qemu -device virtio-net-device,netdev=vmnic \
		-redir udp:10069::69

disco-flash: disco
	openocd -f interface/stlink-v2-1.cfg -f board/stm32756g_eval.cfg \
		-c "program out/build-stm32f746g-disco-fletch/lk.bin reset exit 0x08000000"

FLETCH_TOOL_DIR = third_party/fletch/out/ReleaseIA32/

fletch-tool:
	ninja -C third_party/fletch/ && ninja -C $(FLETCH_TOOL_DIR)

fletch-reset:
	$(FLETCH_TOOL_DIR)fletch quit

fletch-session: fletch-reset fletch-tool
	$(FLETCH_TOOL_DIR)fletch create session sodff with file dart/fletch-settings

%.snap: %.dart fletch-session
	$(FLETCH_TOOL_DIR)fletch export $< to $@ in session sodff

clean:
	rm -rf out

.PHONY: all lk clean disco disco-flash qemu qemu-run fletch-tool fletch-reset fletch-session

# vim: set noexpandtab:

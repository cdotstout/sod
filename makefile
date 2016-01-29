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

dartuino:
	$(MAKE) -f third_party/lk/makefile dartuino-p0-fletch

# build and run lk for qemu with a display
qemu-run: qemu
	qemu-system-arm -machine virt -cpu cortex-a15 \
		-m 12 -kernel out/build-qemu-virt-fletch/lk.elf \
		-device virtio-gpu-device -serial stdio \
		-netdev user,id=vmnic,hostname=qemu -device virtio-net-device,netdev=vmnic \
		-redir udp:10069::69 \
		-redir tcp:4567::4567

# flash the disco board using the checked in version of openocd

UNAME_S = $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  PLATFORM = linux
endif
ifeq ($(UNAME_S),Darwin)
  PLATFORM = mac
endif

OPENOCD_DIR = third_party/openocd/$(PLATFORM)/openocd

disco-flash: disco
	$(OPENOCD_DIR)/bin/openocd \
		-f interface/stlink-v2-1.cfg \
		-f board/stm32756g_eval.cfg \
		--search $(OPENOCD_DIR)/share/openocd/scripts \
		-c "program out/build-stm32f746g-disco-fletch/lk.bin reset exit 0x08000000"

# support for building Fletch snapshots from dart files
DARTINO_TOOL_DIR = third_party/dartino/out/ReleaseIA32/

dartino-tool:
	ninja -C third_party/dartino/ && ninja -C $(DARTINO_TOOL_DIR)

dartino-reset: dartino-tool
	$(DARTINO_TOOL_DIR)fletch quit

dartino-session: dartino-reset
	$(DARTINO_TOOL_DIR)fletch create session sodff with file dart/fletch-settings

dartino-debug-qemu: dartino-session
	$(DARTINO_TOOL_DIR)fletch attach in session sodff tcp_socket 0.0.0.0:4567
	$(DARTINO_TOOL_DIR)fletch debug ${ARGS} in session sodff

%.snap: %.dart dartino-session
	$(DARTINO_TOOL_DIR)fletch export $< to $@ in session sodff

clean:
	rm -rf out
	rm -rf third_party/dartino/out

.PHONY: all lk clean disco disco-flash qemu qemu-run dartino-tool dartino-reset dartino-session

# vim: set noexpandtab:

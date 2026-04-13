CROSS ?= riscv64-elf-

CC      := $(CROSS)gcc
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size
QEMU    ?= qemu-system-riscv64

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/hello-uart.elf
MAP_FILE  := $(BUILD_DIR)/hello-uart.map
DISASM    := $(BUILD_DIR)/hello-uart.disasm

TEST_EBREAK ?= 0
TEST_PANIC  ?= 0

CFLAGS := \
	-Wall -Wextra -Werror \
	-O0 -g3 \
	-ffreestanding -fno-builtin \
	-fno-omit-frame-pointer \
	-march=rv64imac_zicsr_zifencei \
	-mabi=lp64 \
	-mcmodel=medany \
	-mno-relax \
	-msmall-data-limit=0 \
	-Iplatform \
	-Idrivers \
	-DTEST_EBREAK=$(TEST_EBREAK) \
	-DTEST_PANIC=$(TEST_PANIC)

LDFLAGS := \
	-T linker/qemu_virt.ld \
	-nostdlib -nostartfiles \
	-Wl,-Map=$(MAP_FILE)

SRCS_C := \
	app/main.c \
	drivers/virtio_mmio.c \
	drivers/virtqueue.c \
	drivers/virtio_net.c \
	platform/uart.c \
	platform/panic.c \
	platform/trap.c

SRCS_S := \
	platform/start.S \
	platform/trap.S

OBJS := \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/virtio_mmio.o \
	$(BUILD_DIR)/virtqueue.o \
	$(BUILD_DIR)/virtio_net.o \
	$(BUILD_DIR)/uart.o \
	$(BUILD_DIR)/panic.o \
	$(BUILD_DIR)/trap.o \
	$(BUILD_DIR)/start.o \
	$(BUILD_DIR)/trap_asm.o

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: app/main.c drivers/virtio_mmio.h drivers/virtio_net.h platform/panic.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/virtio_mmio.o: drivers/virtio_mmio.c drivers/virtio_mmio.h platform/qemu_virt.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/virtqueue.o: drivers/virtqueue.c drivers/virtqueue.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/virtio_net.o: drivers/virtio_net.c drivers/virtio_net.h drivers/virtio_mmio.h drivers/virtqueue.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/uart.o: platform/uart.c platform/uart.h platform/qemu_virt.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/panic.o: platform/panic.c platform/panic.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/trap.o: platform/trap.c platform/trap.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/start.o: platform/start.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/trap_asm.o: platform/trap.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

run: $(TARGET)
	$(QEMU) \
		-machine virt \
		-m 128M \
		-smp 1 \
		-bios none \
		-display none \
		-serial stdio \
		-monitor none \
		-netdev user,id=net0 \
		-device virtio-net-device,netdev=net0,mac=52:54:00:12:34:56 \
		-device loader,file=$(TARGET),cpu-num=0

debug: $(TARGET)
	$(QEMU) \
		-machine virt \
		-m 128M \
		-smp 1 \
		-bios none \
		-display none \
		-serial stdio \
		-monitor none \
		-S -gdb tcp::1234 \
		-netdev user,id=net0 \
		-device virtio-net-device,netdev=net0,mac=52:54:00:12:34:56 \
		-device loader,file=$(TARGET),cpu-num=0

disasm: $(TARGET)
	$(OBJDUMP) -d $(TARGET) > $(DISASM)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run debug disasm clean

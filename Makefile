CROSS ?= riscv64-elf-

CC      := $(CROSS)gcc
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size
QEMU    ?= qemu-system-riscv64
SUDO    ?= sudo

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/hello-uart.elf
MAP_FILE  := $(BUILD_DIR)/hello-uart.map
DISASM    := $(BUILD_DIR)/hello-uart.disasm

TEST_EBREAK ?= 0
TEST_PANIC  ?= 0
NET          ?= user
TAP_IF       ?= tap0
VMNET_START  ?= 192.168.100.10
VMNET_END    ?= 192.168.100.254
VMNET_MASK   ?= 255.255.255.0

QEMU_NETDEV_user  := user,id=net0
QEMU_NETDEV_tap   := tap,id=net0,ifname=$(TAP_IF),script=no,downscript=no
QEMU_NETDEV_vmnet := vmnet-host,id=net0,start-address=$(VMNET_START),end-address=$(VMNET_END),subnet-mask=$(VMNET_MASK)
QEMU_NETDEV       := $(QEMU_NETDEV_$(NET))

ifeq ($(QEMU_NETDEV),)
$(error Unknown NET='$(NET)'. Use NET=user, NET=tap, or NET=vmnet)
endif

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
	-Inet \
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
	net/eth.c \
	net/arp.c \
	net/checksum.c \
	net/stats.c \
	net/ipv4.c \
	net/icmp.c \
	net/udp.c \
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
	$(BUILD_DIR)/eth.o \
	$(BUILD_DIR)/arp.o \
	$(BUILD_DIR)/checksum.o \
	$(BUILD_DIR)/stats.o \
	$(BUILD_DIR)/ipv4.o \
	$(BUILD_DIR)/icmp.o \
	$(BUILD_DIR)/udp.o \
	$(BUILD_DIR)/uart.o \
	$(BUILD_DIR)/panic.o \
	$(BUILD_DIR)/trap.o \
	$(BUILD_DIR)/start.o \
	$(BUILD_DIR)/trap_asm.o

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: app/main.c drivers/virtio_mmio.h drivers/virtio_net.h net/eth.h net/netif.h net/stats.h platform/panic.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/virtio_mmio.o: drivers/virtio_mmio.c drivers/virtio_mmio.h platform/qemu_virt.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/virtqueue.o: drivers/virtqueue.c drivers/virtqueue.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/virtio_net.o: drivers/virtio_net.c drivers/virtio_net.h drivers/virtio_mmio.h drivers/virtqueue.h net/eth.h net/netif.h net/stats.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/eth.o: net/eth.c net/eth.h net/arp.h net/endian.h net/ipv4.h net/netif.h net/stats.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/arp.o: net/arp.c net/arp.h net/eth.h net/endian.h net/netif.h net/stats.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/checksum.o: net/checksum.c net/checksum.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/stats.o: net/stats.c net/stats.h platform/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ipv4.o: net/ipv4.c net/ipv4.h net/checksum.h net/endian.h net/eth.h net/icmp.h net/netif.h net/stats.h net/udp.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/icmp.o: net/icmp.c net/icmp.h net/checksum.h net/endian.h net/eth.h net/ipv4.h net/netif.h net/stats.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/udp.o: net/udp.c net/udp.h net/checksum.h net/endian.h net/eth.h net/ipv4.h net/netif.h net/stats.h | $(BUILD_DIR)
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
		-netdev $(QEMU_NETDEV) \
		-device virtio-net-device,netdev=net0,mac=52:54:00:12:34:56 \
		-device loader,file=$(TARGET),cpu-num=0

run-tap:
	$(MAKE) run NET=tap

run-vmnet: $(TARGET)
	$(SUDO) $(QEMU) \
		-machine virt \
		-m 128M \
		-smp 1 \
		-bios none \
		-display none \
		-serial stdio \
		-monitor none \
		-netdev $(QEMU_NETDEV_vmnet) \
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
		-netdev $(QEMU_NETDEV) \
		-device virtio-net-device,netdev=net0,mac=52:54:00:12:34:56 \
		-device loader,file=$(TARGET),cpu-num=0

disasm: $(TARGET)
	$(OBJDUMP) -d $(TARGET) > $(DISASM)

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all run run-tap run-vmnet debug disasm clean

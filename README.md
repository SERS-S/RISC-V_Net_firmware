# RISC-V Net Firmware (Educational project)

![Target](https://img.shields.io/badge/target-QEMU%20virt-2f80ed)
![ISA](https://img.shields.io/badge/ISA-RISC--V-283272)
![Mode](https://img.shields.io/badge/mode-bare--metal-c0392b)
![Network](https://img.shields.io/badge/network-virtio--net-16a085)
![Stack](https://img.shields.io/badge/stack-ARP%20%7C%20ICMP%20%7C%20UDP-27ae60)
![Status](https://img.shields.io/badge/status-MVP-f39c12)

Учебная bare-metal прошивка для RISC-V в QEMU `virt` с собственным
`virtio-mmio` сетевым драйвером и минимальным IPv4-стеком, реализующим ARP,
ICMP echo и UDP echo.

Проект запускается в `qemu-system-riscv64` без Linux и без полноценной ОС:
ELF загружается напрямую в виртуальную плату `virt`, прошивка инициализирует
UART, находит `virtio-net` устройство через MMIO, поднимает RX/TX virtqueue и
обрабатывает Ethernet-кадры своим минимальным сетевым стеком.

## Что реализовано

![Boot](https://img.shields.io/badge/boot-ELF%20loader-34495e)
![Driver](https://img.shields.io/badge/driver-virtio--mmio-8e44ad)
![ARP](https://img.shields.io/badge/ARP-reply%20ready-27ae60)
![ICMP](https://img.shields.io/badge/ICMP-ping%20ready-27ae60)
![UDP](https://img.shields.io/badge/UDP-echo%2012345-27ae60)

- Bare-metal старт на QEMU `virt`
- UART логирование.
- Trap/panic каркас
- `virtio-mmio` device scan
- `virtio-net` legacy MMIO init
- RX/TX virtqueue polling
- Ethernet input/output
- ARP responder и небольшой ARP cache
- IPv4 input/output без options
- Drop фрагментированных IPv4 пакетов
- ICMP Echo Request
- UDP echo server на порту `12345`
- Минимальное логирование сетевых действий
- Запуск через QEMU macOS `vmnet-host`

## Фиксированная конфигурация

Гость:

- MAC: `52:54:00:12:34:56`
- IPv4: `192.168.100.2`
- mask: `255.255.255.0`
- UDP echo port: `12345`

macOS `vmnet-host`:

- host bridge обычно: `bridge100`
- host IPv4: `192.168.100.10/24`
- vmnet range: `192.168.100.10` - `192.168.100.254`

## Архитектура

```text
app/
  main.c                 main loop: init + virtio_net_poll_rx/tx + stats

drivers/
  virtio_mmio.c/.h       virtio-mmio регистры и device scan
  virtqueue.c/.h         split virtqueue helpers
  virtio_net.c/.h        virtio-net init, RX/TX buffers, netif setup

linker/
  qemu_virt.ld           bare-metal linker script

net/
  endian.h               htons/ntohs/htonl/ntohl helpers
  checksum.c/.h          Internet checksum
  netif.h                MAC/IP/TX abstraction
  eth.c/.h               Ethernet second layer
  arp.c/.h               ARP parser, responder, cache
  ipv4.c/.h              IPv4 validation and output
  icmp.c/.h              ICMP echo
  udp.c/.h               UDP parser/checksum/echo
  stats.c/.h             minimal counters

platform/
  start.S                entry point, stack, .bss clear
  trap.S/.c/.h           trap handling
  uart.c/.h              UART output
  panic.c/.h             panic path
  csr.h                  CSR helpers
  qemu_virt.h            QEMU virt constants
```

Разделение по слоям:

- `platform/` отвечает за запуск платформы
- `drivers/` (драйвера) отвечает за доступ к сетевому устройству
- `net/` отвечает за интерпретирование Ethernet/IP пакетов
- `app/` связывает инициализацию и event loop

RX pipeline:

```text
virtio_net_poll_rx()
  -> eth_input()
    -> arp_input()
    -> ipv4_input()
      -> icmp_input()
      -> udp_input()
        -> ipv4_output()
          -> eth_output()
            -> virtio_net_tx()
```

## Зависимости

Нужны:

- `qemu-system-riscv64`
- `riscv64-elf-gcc`
- `riscv64-elf-objdump`
- `riscv64-elf-size`
- `tcpdump`
- `arping`
- `ping`
- `nc` или `socat`

На macOS через Homebrew используются пакеты QEMU и bare-metal RISC-V
toolchain. В данном проекте `Makefile` по умолчанию ожидает prefix:

```bash
riscv64-elf-
```
> [!WARNING]
> Иначе нужно передавать другой prefix: `make CROSS=riscv64-unknown-elf-`

## Сборка

```bash
make
```

Результат:

```text
build/hello-uart.elf
build/hello-uart.map
```

Очистка:

```bash
make clean
```

Дизассемблирование:

```bash
make disasm
```

## Запуск

### Обычный local-test

```bash
make run
```

По умолчанию используется QEMU `user` networking:

```text
-netdev user,id=net0
```

Этот режим подходит для проверки загрузки и инициализации `virtio-net`, но не
подходит для полноценного ARP теста с хоста, потому что host не находится в
одном Ethernet сегменте с гостем.

### macOS: vmnet-host

Для ARP, ping и UDP echo на macOS используется `vmnet-host`.

```bash
make run-vmnet
```

Так QEMU запускается через `sudo`, потому что `vmnet-host` требует повышенных
прав:

```text
-netdev vmnet-host,id=net0,start-address=192.168.100.10,end-address=192.168.100.254,subnet-mask=255.255.255.0
```

После запуска можно проверить интерфейс по команде:

```bash
ifconfig bridge100
```

`bridge100` должен иметь адрес из `192.168.100.0/24`, например:

```text
inet 192.168.100.10 netmask 0xffffff00 broadcast 192.168.100.255
```

## Тестирование сети

![ARP test](https://img.shields.io/badge/test-arping-2980b9)
![Ping test](https://img.shields.io/badge/test-ping-2980b9)
![UDP test](https://img.shields.io/badge/test-nc%20%7C%20socat-2980b9)

Команды ниже предполагают macOS `vmnet-host` и интерфейс `bridge100`.

### ARP

macOS:

```bash
sudo arping -i bridge100 192.168.100.2
```

Ожидаемый результат:

```text
60 bytes from 52:54:00:12:34:56 (192.168.100.2)
```

### ICMP echo

```bash
ping 192.168.100.2
```

Ожидаемо: ответ от `192.168.100.2`.

### UDP echo

Через `nc`:

```bash
printf 'abc123' | nc -u -w1 192.168.100.2 12345
```

Через `socat`:

```bash
printf 'abc123' | socat - UDP:192.168.100.2:12345
```

## Логи и статистика

UART печатает boot log и компактную сетевую статистику:

```text
net-stats: rx=0x... tx=0x... arp=0x... ip=0x... icmp=0x... udp=0x... bad_len=0x... bad_sum=0x... wrong_dst=0x... unsup=0x... tx_err=0x...
```

Счётчики:

- `rx` - принятые Ethernet frames из virtio-net.
- `tx` - отправленные Ethernet frames.
- `arp` - обработанные ARP frames.
- `ip` - обработанные IPv4 packets.
- `icmp` - обработанные ICMP packets.
- `udp` - обработанные UDP packets.
- `bad_len` - короткая или некорректная длина.
- `bad_sum` - ошибка checksum.
- `wrong_dst` - пакет не для нашего MAC/IP/ARP target.
- `unsup` - неподдержанный ethertype/protocol/type/port.
- `tx_err` - ошибка отправки.

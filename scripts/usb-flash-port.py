#!/usr/bin/env python3
"""Resolve a K518 ROM port, asking our composite HID/CDC firmware to reboot if needed."""
import sys
import time
import serial
from serial.tools import list_ports

port = sys.argv[1]
devices = list(list_ports.comports())
current = next((p for p in devices if p.device == port or
                p.device == port.replace('/cu.', '/tty.')), None)
if current and current.vid == 0x303A and current.pid in (0x4005, 0x4009):
    with serial.Serial(port, 115200, timeout=0.2) as connection:
        connection.write(b'bootloader\n')
        connection.flush()
    for _ in range(100):
        time.sleep(0.1)
        rom = [p for p in list_ports.comports()
               if p.vid == 0x303A and p.pid in (0x1001, 0x0009)
               and p.location == current.location]
        if rom:
            print(rom[0].device)
            break
    else:
        sys.exit('ROM port did not appear; hold BOOT while reconnecting the S3 USB cable')
else:
    print(port)

#!/usr/bin/python3

import hid
import sys
import uuid
import os.path

# Unique ID for the station, helps if there are multiple units transmitting
# with the same channel ID. The receiving station picks one and pairs with
# it instead of bouncing back and forth.
sensorid = uuid.getnode() & 0x3ff

if len(sys.argv) != 4:
    print(f'Usage: {os.path.basename(__file__)} <A|B|C> <temp in C> <rh%>')
    sys.exit(1)

# Just find the first device and use it.
devices = [n for n in hid.enumerate() if (n['manufacturer_string'], n['product_string']) == ('russd@asu.edu', 'RF')]
dev = hid.device()
dev.open_path(devices[0]['path'])

# Parse command line arguments
channel = sys.argv[1].upper()
temp_c = float(sys.argv[2])
rh = int(round(float(sys.argv[3])))
lowbatt = 0

# Transform command line arguments into message bytes
ch = {'A': 0xc0, 'B': 0x80, 'C': 0x00}[channel] | (sensorid >> 8)
temp = int(1000 + round(temp_c * 10)) # 100.0 + 69.0 C
sensorid &= 0xff
status = 0x04 | (0x80 if lowbatt else 0x40)
if rh.bit_count() & 1: # Parity
    rh |= 0x80
temp0 = temp >> 7
if temp0.bit_count() & 1: # Parity
    temp0 |= 0x80
temp1 = temp & 0x7f
if temp1.bit_count() & 1: # Parity
    temp1 |= 0x80

# Calculate modulo 256 checksum
msg = [ch, sensorid, status, rh, temp0, temp1]
msg.append(sum(msg) & 0xff)


# Byte 0: HID report ID 1
# Byte 1: RF Protocol 8
# Byte 2: Message length in bits
# Bytes 3-9: Message (including final byte for trailing zero bit)
msg = [1, 8, len(msg) * 8 + 1, *msg, 0]

# Send it 3 times
for i in range(0, 3):
    dev.write(msg)
    dev.read(3)

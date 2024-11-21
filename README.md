# ATtiny V-USB Firmware for 434MHz RF Protocols

This firmware facilites easily sending messages to various 434MHz RF devices.
The firmware currently supports the following weather station devices:

* Oregon Scientific v1.0 Devices
* Oregon Scientific v2.1 Devices
* Oregon Scientific v3.0 Devices
* Oregon Scientific SL-109H
* Ambient Weather F007TH
* Ambient Weather WH2B
* AcuRite VN1TX
* AcuRite 00592TXR 
* AcuRite 00955
* AcuRite 00964TX 
* AcuRite 00782W3
* AcuRite 00606TX
* AcuRite 00609A1TX

As documented in https://www.osengr.org/Articles/OS-RF-Protocols-IV.pdf

Note that some receivers have very tight timings (eg, 1 part in 10^6) on
*when* messages can be sent and are otherwise ignored, such as the THGR810.
Supporting such a receiver would either require  a crystal oscillator and
the ability to specify a scheduled transmission time or the ability to
accurately specify during which frame a USB request is sent.

The firmware is designed for use on Digispark hardware connected to a simple
434MHz RF transmitter module such as the Sparkfun WRL-10534. The Digispark
PB1 (MOSI) signal should be connected to the RF transmitter's data line. The
Digispark LED will flash when data is being transmitted.

A config entry is also provided for use with the vmeiosis bootloader in the
`configs/vme` path (`CONFIG=vme`).

The firmware can be easily modified to support any other AVR platforms with
V-USB and USI support.

# Usage

The device enumerates on the USB bus as a HID device with a 18 byte output
report and a 2 byte input report. Messages are sent via the output report and
status is queried via the input report. Messages should not be sent before the
previous message has completed as indicated by the status message.

## Message Format

| Index     | Description
|-----------|------------------------
| Byte 0    | Report ID (1)
| Byte 1    | RF Protocol (see table)
| Byte 2    | Bit length
| Byte 3-17 | Message bits, MSB first

## RF Protocols

The firmware supports various protocol. Each protocol includes a data rate,
a preamble, and a pattern for sending message bits.

| Value | Description
|-------|------------
| 0     | Oregon Scientific v1.0
| 1     | Oregon Scientific v2.1
| 2     | Oregon Scientific v3.0
| 3     | AcuRite PSM devices (00955/00964TX/00782W3/00606TX/00609A1TX)
| 4     | Acurite 00964TX
| 5     | Acurite 00609A1TX
| 6     | Ambient Weather F007TH
| 7     | Ambient Weather WH2B
| 8     | Acurite PWM devices (00592TXR/VN1TX)

The Oregon Scientific SL-109H also uses the Acurite PSM protocol.

## Status Format

| Index     | Description
|-----------|------------------------
| Byte 0    | Report ID (1)
| Byte 1    | Status byte

The status byte has bit 7 set if the firmware is currently busy. The low bits
of the status byte indicate the RF protocol value of the last message sent.

The status message can be read directly as a feature report, or via the
interrupt endpoint. The interrupt will be activated when the message has
finished sending. In this case checking the status bit is not necessary.

## Example Usage

A sample 57 bit message for the AcuRite 00592TXR (RF protocol 8):

    `ad bc 44 96 09 fc 48 00`

```
import hid

devices = [n for n in hid.enumerate() if (n['manufacturer_string'], n['product_string']) == ('russd@asu.edu', 'RF')]
dev = hid.device()
dev.open_path(devices[0]['path'])

for i in range(0, 3):
    dev.write([1, 8, 57, 0xad 0xbc 0x44 0x96 0x09 0xfc 0x48, 0x00])
    dev.read(3) # Wait for completion interrupt
```

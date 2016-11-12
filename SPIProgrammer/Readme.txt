    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.


The files in this directory enable the construction of a simple, yet powerful SPI programming interface for the Spansion S25FL064A flash memory device.
This memory device stores all of the modem's non-volatile settings and firmware.

This tool allows the entire device to be read and written.
It also allows selective reads/writes to specified memory addresses.

This enables full device backups to be taken and restored, and also enables enables settings areas to be updated without having to reporgram the entire device.

The SPI programmer interface utilises a PIC16F88 (other compatible PICs could be used) to provide the bridge between RS232 and SPI.
A serial port will be needed on the PC, which will need to be converted to TTL voltage levels via a MAX232 or similar device.
Alternatively, a USB Serial converter could be used, and may even be preferable if the USB Serial converter directly provides TTL voltage levels.

The file config.inc describes the pin mapping between the PIC and the S25FL064A. Refer to the S25FL064A data sheet for information on the pin assignment of the flash memory device.

Once the PIC has been programmed, and connected to the PC serial port, verify that it is working correctly by opening a dumb terminal at 115200 1n8, and verify that the string "Hello" is displayed when the PIC is reset or powered on. If this is working, then proceed to use the spi_prog PC utility to communicate with the programmer.


EXAMPLES

To show the help and usage information.
spi_prog

To communicate with the programmer on port /dev/ttyUSB0, and read back the flash memory device ID
spi_prog /dev/ttyUSB0

To read the entire device (full backup)
spi_prog /dev/ttyUSB0 -r 0x000000 0x800000 memory_dump_file.img

To write data, restoring a full backup
spi_prog /dev/ttyUSB0 -w 0x000000 memory_dump_file.img

To erase the entire device
spi_prog /dev/ttyUSB0 -e



To read the dynamic settings area
spi_prog /dev/ttyUSB0 -r 0x7F0000 0x010000 dynamic_settings.img

To write the dynamic settings area
spi_prog /dev/ttyUSB0 -w 0x7F0000 dynamic_settings.img


To read the permanent settings area
spi_prog /dev/ttyUSB0 -r 0x010000 0x010000 permanent_settings.img

To write the permanent settings area
spi_prog /dev/ttyUSB0 -w 0x010000 permanent_settings.img


NOTES
spi_prog is quite slow by modern standards due to fixed baud rate of 115200.
Estimated transfer time for full 8MB memory image is 8*(2^20)/(115200/10)=728 Seconds ~= 12 Minutes
It is recommended that a full memory backup be taken before experimenting.
As most changes thereafter will involve modifying the settings areas, the small transfer size of 64kB
can be read/written in approximately 6 seconds, which isn't so much of an issue.


SEE ALSO
For further information on the memory layout, see Notes/MemoryLayout.txt
The extractmemorydump tool can be used to extract the various regions from a within full memory backup file.

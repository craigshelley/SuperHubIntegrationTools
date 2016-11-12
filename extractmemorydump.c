/*
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>



#define MEMSIZE         0x800000

#define BOOTLOADER_ADDR 0x000000
#define BOOTLOADER_SIZE 0X010000

#define PERMANENT_ADDR  0x010000
#define PERMANENT_SIZE  0X010000

#define DYNAMIC_ADDR    0x7F0000
#define DYNAMIC_SIZE    0X010000

#define FIRMWARE_ADDR   0x020000
#define FIRMWARE_STEP   0x010000

#define FIRMWARE_MAGIC  0xA0E7
#define FIRMWARE_HSIZE  0X005C


struct firmware_header {
	unsigned int Signature;
	unsigned int Control;
	unsigned int Major_Rev;
	unsigned int Minor_Rev;
	struct tm Build_Time;
	unsigned int File_Length;
	unsigned int Load_Address;
	char Filename[64];
	unsigned int Header_CRC;
	unsigned int Body_CRC;
};

void ExtractHeader(unsigned char * buf, struct firmware_header *hdr) {
	hdr->Signature=(buf[0]<<8) | (buf[1]<<0);
	hdr->Control=(buf[2]<<8) | (buf[3]<<0);
	hdr->Major_Rev=(buf[4]<<8) | (buf[5]<<0);
	hdr->Minor_Rev=(buf[6]<<8) | (buf[7]<<0);
	time_t tim=(buf[8]<<24) | (buf[9]<<16) | (buf[10]<<8) | (buf[11]<<0);
	gmtime_r(&tim, &hdr->Build_Time);
	hdr->File_Length=(buf[12]<<24) | (buf[13]<<16) | (buf[14]<<8) | (buf[15]<<0);
	hdr->Load_Address=(buf[16]<<24) | (buf[17]<<16) | (buf[18]<<8) | (buf[19]<<0);
	int i;
	for (i=0; i<64; i++) {
		hdr->Filename[i]=buf[20+i];
	}
	hdr->Header_CRC=(buf[84]<<8) | (buf[85]<<0);
	hdr->Body_CRC=(buf[88]<<24) | (buf[89]<<16) | (buf[90]<<8) | (buf[91]<<0);
}

void DisplayHeader(struct firmware_header *hdr) {
	printf("   Signature: %04x\n", hdr->Signature);
	printf("     Control: %04x\n", hdr->Control);
	printf("   Major Rev: %04x\n", hdr->Major_Rev);
	printf("   Minor Rev: %04x\n", hdr->Minor_Rev);
	printf("  Build Time: %04i/%i/%i %02i:%02i:%02i Z\n", hdr->Build_Time.tm_year+1900, hdr->Build_Time.tm_mon+1, hdr->Build_Time.tm_mday, hdr->Build_Time.tm_hour, hdr->Build_Time.tm_min, hdr->Build_Time.tm_sec);
	printf(" File Length: %i bytes\n", hdr->File_Length);
	printf("Load Address: %08x\n", hdr->Load_Address);
	printf("    Filename: %s\n", hdr->Filename);
	printf("         HCS: %04x\n", hdr->Header_CRC);
	printf("         CRC: %08x\n", hdr->Body_CRC);
}


unsigned int CalculateCRC(unsigned char *buf, unsigned int len, unsigned int init, unsigned int poly, unsigned int final) {
	unsigned int remainder;
	unsigned int bitno;

	remainder=init^((buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | (buf[3]<<0));

	for (bitno=32; bitno<len*8+32; bitno++) {
		/*Perform the long division if MSb is set*/
		if (remainder&0x80000000) {
			remainder=(remainder<<1)^poly;
		} else {
			remainder<<=1;
		}
		/*Add more bits from the bit stream*/
		if (bitno<len*8) {
			remainder^=(buf[bitno/8]>>(7-(bitno%8)))&1;
		}
	}

	return remainder^final;
}

int main(int argc, char * argv[]) {
	char filename[FILENAME_MAX];
	int ofd;
	if (argc!=2) {
		printf("%s memorydumpfile\n", argv[0]);
		return -1;
	}

	unsigned char * buf=(unsigned char *)malloc(MEMSIZE);

	if (buf==NULL) {
		printf("Unable to allocate memory\n");
		return -2;
	}

	int fd=open(argv[1], O_RDONLY);
	if (fd<=0) {
		printf("Unable to open file\n");
		return -3;
	}

	if (read(fd, buf, MEMSIZE) != MEMSIZE) {
		printf("Unable to read %i bytes\n", MEMSIZE);
		return -4;
	}

	close(fd);


	int i;

	/* Find the end of the bootloader*/
	for (i=BOOTLOADER_SIZE-1; i>BOOTLOADER_ADDR && buf[i]==0xFF; i--);

	/*Round up to the nearest 4 byte boundary*/
	i+=4-(i%4);

	snprintf(filename, FILENAME_MAX, "%s.bootloader", argv[1]);
	printf("0x%06x - 0x%06x Bootloader:          0x%06X bytes > %s\n", BOOTLOADER_ADDR, BOOTLOADER_ADDR+i, i, filename);
	ofd=open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (ofd<=0) {
		printf("Unable to open file for writing\n");
		return -5;
	}
	write(ofd, &buf[BOOTLOADER_ADDR], i);
	close(ofd);


	/* Extract the Permanent Settings*/
	snprintf(filename, FILENAME_MAX, "%s.permanent", argv[1]);
	printf("0x%06x - 0x%06x Permanent Settings:  0x%06X bytes > %s\n", PERMANENT_ADDR, PERMANENT_ADDR+PERMANENT_SIZE, PERMANENT_SIZE, filename);
	ofd=open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (ofd<=0) {
		printf("Unable to open file for writing\n");
		return -5;
	}
	write(ofd, &buf[PERMANENT_ADDR], PERMANENT_SIZE);
	close(ofd);


	/* Extract the Dynamic Settings*/
	snprintf(filename, FILENAME_MAX, "%s.dynamic", argv[1]);
	printf("0x%06x - 0x%06x Dynamic Settings:    0x%06X bytes > %s\n", DYNAMIC_ADDR, DYNAMIC_ADDR+DYNAMIC_SIZE, DYNAMIC_SIZE, filename);
	ofd=open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (ofd<=0) {
		printf("Unable to open file for writing\n");
		return -5;
	}
	write(ofd, &buf[DYNAMIC_ADDR], DYNAMIC_SIZE);
	close(ofd);

	unsigned int imgaddr;
	unsigned int firmware_no=0;
	for (imgaddr=FIRMWARE_ADDR; imgaddr+FIRMWARE_STEP<MEMSIZE; imgaddr+=FIRMWARE_STEP-(imgaddr%FIRMWARE_STEP)) {
		unsigned int magic=(buf[imgaddr]<<8)  | buf[imgaddr+1];
		if (magic==FIRMWARE_MAGIC) {
			struct firmware_header hdr;
			ExtractHeader(&buf[imgaddr], &hdr);			
			unsigned int Header_CRC=CalculateCRC(&buf[imgaddr], FIRMWARE_HSIZE-8, 0xFFFF0000, 0x10210000, 0xFFFF0000)>>16;

			if (Header_CRC != hdr.Header_CRC) {
				printf("0x%06x Invalid header\n", imgaddr);
				continue;
			}

			firmware_no++;

			printf("\n\n");
			printf("Image %i Program Header:\n", firmware_no);
			DisplayHeader(&hdr);

			if (hdr.File_Length > MEMSIZE-imgaddr) {
				printf("ERROR: Image length exceeds remaining memory size\n");
				continue;
			}
			
			unsigned int Body_CRC=CalculateCRC(&buf[imgaddr+FIRMWARE_HSIZE], hdr.File_Length, 0xFFFFFFFF, 0x04C11DB7, 0xFFFFFFFF);
			if (Body_CRC != hdr.Body_CRC) {
				printf("ERROR: Body CRC Mismatch\n");
				continue;
			}

			/* Extract the Firmware Image Settings*/
			snprintf(filename, FILENAME_MAX, "%s.image_0x%06x", argv[1], imgaddr);
			printf("0x%06x - 0x%06x Image %i:    0x%06X bytes > %s\n", imgaddr, imgaddr+hdr.File_Length+FIRMWARE_HSIZE, firmware_no, hdr.File_Length+FIRMWARE_HSIZE, filename);
			ofd=open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (ofd<=0) {
				printf("Unable to open file for writing\n");
				return -5;
			}
			write(ofd, &buf[imgaddr], hdr.File_Length+FIRMWARE_HSIZE);
			close(ofd);
			imgaddr+=+hdr.File_Length+FIRMWARE_HSIZE;
		}
	}

}

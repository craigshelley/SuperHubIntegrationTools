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


#define PARAM_BLOCK_SIZE   0x10000
#define PARAM_BLOCK_OFFSET 0x000CA

unsigned int settings_checksum(unsigned char * buf, int len) {
	unsigned int sum=0x0;
	unsigned int schedule=0;
	unsigned int i;
	for(i=0; i<len; i++) {
		switch (i%4) {
			case 0:
				schedule=buf[i]<<8;
				break;
			case 1:
				schedule<<=16;
				schedule|=buf[i]<<16;
				break;
			case 2:;
				schedule|=buf[i]<<8;
				break;
			case 3:
				schedule|=buf[i];
				sum+=schedule;
				schedule=0;
				break;
		}
						
	}
	sum+=schedule;

	return ~sum;
}

int main (int argc, char * argv[]) {
	int i;
	char dirname[FILENAME_MAX];
	char filename[FILENAME_MAX];


	unsigned char ibuf[PARAM_BLOCK_SIZE];
	unsigned int length;

	if (argc!=2) {
		printf("Usage: %s filename_to_unpack\n", argv[0]);
		return 1;
	}

	printf("File:   %s\n", argv[1]);
	int fd=open(argv[1], O_RDONLY);

	if (fd<=0) {
		printf("ERROR: Unable to open file\n");
		return 1;
	}

	length=read(fd, ibuf, PARAM_BLOCK_SIZE);
	close(fd);
	
	if (length==PARAM_BLOCK_SIZE || length<8) {
		printf("ERROR: Invalid number of bytes read %i\n", length);
		return 1;
	}

	unsigned int lengthheader=(ibuf[0]<<24) | (ibuf[1]<<16) | (ibuf[2]<<8) | (ibuf[3]<<0) ;
	unsigned int crc=(ibuf[4]<<24) | (ibuf[5]<<16) | (ibuf[6]<<8) | (ibuf[7]<<0) ;


	printf("Length: 0x%08x\n", lengthheader);
	printf("CRC:    0x%08x\n", crc);

	if (lengthheader != length) {
		printf("  ERROR: Settings length header does not match file size.\n");
		return 1;
	}

	if (settings_checksum(ibuf, length) != 0) {
		printf("ERROR: Invalid CRC\n");
		return 2;
	}

	sprintf(dirname, "%s_settings", argv[1]);
	mkdir(dirname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

	int bufptr=8;
	int objno=0;
	while (bufptr<length) {
		unsigned int confsize=(ibuf[bufptr]<<8) | (ibuf[bufptr+1]<<0);
		char magic[9];
		int magptri=0;
		int magptro=0;

		if (confsize==0) {
			printf("ERROR: Zero sized configuration object encountered.\n");
			return 3;
		}

		for (magptri=0; magptri<4; magptri++) {
			if (
				(ibuf[bufptr+2+magptri] >= '0' && ibuf[bufptr+2+magptri] <= '9') ||
				(ibuf[bufptr+2+magptri] >= 'A' && ibuf[bufptr+2+magptri] <= 'Z') ||
				(ibuf[bufptr+2+magptri] >= 'a' && ibuf[bufptr+2+magptri] <= 'z')) {

				magic[magptro]=ibuf[bufptr+2+magptri];
				magptro++;
			} else {
				sprintf(&magic[magptro], "%02X", ibuf[bufptr+2+magptri]);
				magptro+=2;
			}
		}
		magic[magptro]='\0';

		sprintf(filename, "%s/%03i_%s.cobj", dirname, objno, magic);



		printf("Object: %03i  Addr: 0x%06x  Size: %5i  Magic: %-8s > '%s'\n", objno, bufptr, confsize, magic, filename);

		int ofd=open(filename, O_WRONLY | O_CREAT | O_TRUNC,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (ofd<=0) {
			printf("  ERROR: Unable to open file for writing.\n");
			return 5;
		} 
		write(ofd, &ibuf[bufptr], confsize);
		close(ofd);


		objno++;
		bufptr+=confsize;
	}
}



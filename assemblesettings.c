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
#include <string.h>


#define PARAM_BLOCK_SIZE   0x10000

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


	unsigned char param_block[PARAM_BLOCK_SIZE];
	unsigned char ibuf[PARAM_BLOCK_SIZE];
	unsigned int length;
	unsigned int optr=8;

	memset(param_block, 0, PARAM_BLOCK_SIZE);

	if (argc<3) {
		printf("Usage: %s settings_object_file_01 settings_object_file_02 settings_object_file_03 ... parameter_block_output_file\n", argv[0]);
		return 1;
	}

	/*Read and check the input files*/
	for (i=1; i<argc-1; i++) {
		printf("Reading:   %s\n", argv[i]);
		int fd=open(argv[i], O_RDONLY);

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

		unsigned int lengthheader=(ibuf[0]<<8) | (ibuf[1]<<0);

		if (lengthheader != length) {
			printf("ERROR: Settings length header does not match file size. lengthheader=0x%04x filesize=0x%04x\n", lengthheader, length);
			return 1;
		}

		if (optr+length > PARAM_BLOCK_SIZE - 8) {
			printf("ERROR: Maximum parameter block size exceeded.\n");
			return 1;
		}

		/*Copy the data into the parameter block*/
		memcpy(&param_block[optr], ibuf, length);
		optr+=length;




	}
	printf("\n");
	
	/*Write the length and headers*/
	param_block[0]=(optr>>24)&0xFF;
	param_block[1]=(optr>>16)&0xFF;
	param_block[2]=(optr>>8)&0xFF;
	param_block[3]=(optr>>0)&0xFF;

	/*Calculate the checksum (includeing the length header, which is before the blank checksum field)*/
	unsigned int checksum=settings_checksum(param_block, optr);

	param_block[4]=(checksum>>24)&0xFF;
	param_block[5]=(checksum>>16)&0xFF;
	param_block[6]=(checksum>>8)&0xFF;
	param_block[7]=(checksum>>0)&0xFF;

	printf("Length:    0x%08x\n", optr);
	printf("Checksum:  0x%08x\n", checksum);

	/*Save the file*/
	printf("Writing:   %s\n", argv[argc-1]);
	int ofd=open(argv[argc-1], O_WRONLY | O_CREAT | O_TRUNC,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (ofd<=0) {
		printf("  ERROR: Unable to open file for writing.\n");
		return 5;
	} 

	write(ofd, param_block, optr);
	close(ofd);
}



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


	unsigned char ibuf[PARAM_BLOCK_SIZE];
	unsigned int blocksize;
	unsigned int allocatedblocks_bf;
	unsigned int allocatedblocks;

	if (argc!=2) {
		printf("Usage: %s filename_to_unpack\n", argv[0]);
		return 1;
	}

	int fd=open(argv[1], O_RDONLY);

	if (fd<=0) {
		printf("ERROR: Unable to open file\n");
		return 1;
	}

	if (read(fd, ibuf, PARAM_BLOCK_SIZE)!=PARAM_BLOCK_SIZE) {
		printf("ERROR: Unable to read data, expected 65536 bytes\n");
		return 1;
	}



	blocksize= (ibuf[PARAM_BLOCK_SIZE-8]<<24) | (ibuf[PARAM_BLOCK_SIZE-7]<<16) | (ibuf[PARAM_BLOCK_SIZE-6]<<8) | (ibuf[PARAM_BLOCK_SIZE-5]<<0);
	allocatedblocks_bf=(ibuf[PARAM_BLOCK_SIZE-4]<<24) | (ibuf[PARAM_BLOCK_SIZE-3]<<16) | (ibuf[PARAM_BLOCK_SIZE-2]<<8) | (ibuf[PARAM_BLOCK_SIZE-1]<<0);
	allocatedblocks=0;
	for (i=0; i<32; i++) {
		if ((allocatedblocks_bf & (1<<i))==0) {
			allocatedblocks++;
		} else {
			break;
		}
	}

	printf("Parameter Block Size:  0x%08x\n", blocksize);
	printf("Allocated Block Count: 0x%08x (%i blocks allocated)\n", allocatedblocks_bf, allocatedblocks);

	if (allocatedblocks==0) {
		printf("ERROR: Allocated block count of zero is invalid.\n");
		return 0;
	}

	if ((blocksize * allocatedblocks)>PARAM_BLOCK_SIZE) {
		printf("ERROR: Total allocated blocks exceed parameter area size\n");
		return 0;
	}

	int blockno=0;
	for (blockno=0; blockno<allocatedblocks; blockno++) {
		int addr=(blockno*blocksize)+PARAM_BLOCK_OFFSET;

		printf("\n0x%04X Settings Block %i:\n", addr, blockno);

		unsigned int length=(ibuf[addr+0]<<24) | (ibuf[addr+1]<<16) | (ibuf[addr+2]<<8) | (ibuf[addr+3]<<0) ;
		unsigned int crc=(ibuf[addr+4]<<24) | (ibuf[addr+5]<<16) | (ibuf[addr+6]<<8) | (ibuf[addr+7]<<0) ;

		printf("  Length: 0x%08x\n", length);
		printf("  CRC:    0x%08x\n", crc);

		if ((length+PARAM_BLOCK_OFFSET)>blocksize) {
			printf("  ERROR: Settings length exceeds block size\n");
			continue;
		}

		if (length<8) {
			printf("  ERROR: Settings length too short\n");
			continue;
		}

		if (settings_checksum(&ibuf[addr], length) == 0) {
			printf("  Checksum is valid\n");
		} else {
			printf("  ERROR: Invalid Checksum\n");
			continue;
		}

		char outfile[256];
		sprintf(outfile, "%s.param_%02d", argv[1], blockno);
		printf("  Writing to file \"%s\"\n", outfile);

		int ofd=open(outfile, O_WRONLY | O_CREAT | O_TRUNC,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (ofd<=0) {
			printf("  ERROR: Unable to open file for writing.\n");
			continue;
		}

		write(ofd, &ibuf[addr], length);
		close(ofd);

	}


	close(fd);
}



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


#define PARAM_BLOCK_SIZE          0x10000
#define PARAM_BLOCK_FOOTER_SIZE   0x00008
#define PARAM_BLOCK_OFFSET        0x000CA

int main (int argc, char * argv[]) {
	int i;

	unsigned char ibuf[PARAM_BLOCK_SIZE];
	unsigned int blocksize;
	unsigned int allocatedblocks_bf;
	unsigned int allocatedblocks=0;
	unsigned int lastblock=0;
	unsigned int size[256];
	int ifd[256];

	if (argc<3) {
		printf("Usage: %s filename_to_pack1 filename_to_pack2 filename_to_pack3 ... output_filename\n", argv[0]);
		return 1;
	}

	/*Clear the buffer*/
	memset(ibuf, 0xFF, PARAM_BLOCK_SIZE);

	/*Open the files and determine the block size*/
	for (i=0; i<argc-2; i++) {
		printf("Block %02i, Opening %s\n", i, argv[i+1]);
		ifd[i]=open(argv[i+1], O_RDONLY);

		if (ifd[i]<=0) {
			printf("ERROR: Unable to open file for reading: %s\n", argv[i+1]);
			return 1;
		}

		struct stat stbuf;
		fstat(ifd[i], &stbuf);
		size[i]=stbuf.st_size;
		if (blocksize<size[i]) {
			blocksize=size[i];
		}
		lastblock++;
	}

	unsigned int firstblock=0;

	for (;;) {

		/*Determine the blocksize*/
		blocksize=0;
		for (i=firstblock; i<lastblock; i++) {
			if (blocksize<size[lastblock-1]) {
				blocksize=size[lastblock-1];
			}
		}

		/*Round block size up to 0x4000 byte boundary*/
		blocksize=((blocksize-1)&~0x3FFF) + 0x4000;

		/*Does everything fit?*/
		if ((PARAM_BLOCK_OFFSET + blocksize*(lastblock-firstblock -1) + size[lastblock-1]) <= (PARAM_BLOCK_SIZE-PARAM_BLOCK_FOOTER_SIZE)) {
			break;
		}

		printf("WARNING: Last block will overhang end of memory space. Discarding block %02i.\n", firstblock);
		firstblock++;

		if ((lastblock-firstblock) < 1 ) {
			printf("ERROR: Insufficient memory to store parameters\n");
			return -1;
		}
	}
	printf("Blocksize: 0x%08x\n", blocksize);

	unsigned int bufptr=PARAM_BLOCK_OFFSET;
	for (i=firstblock; i<lastblock; i++) {
		int bytes;
		bytes=read(ifd[i], &ibuf[bufptr], PARAM_BLOCK_SIZE-PARAM_BLOCK_FOOTER_SIZE-bufptr);
		if(bytes<1) {
			printf("ERROR: Unable to read data for block %02i\n", i);
			return 1;
		}

		printf("Block %02i, Bytes Read: 0x%08x\n", i, bytes);

		bufptr+=blocksize;
		allocatedblocks++;
	}

	/* Close the files */
	for (i=0; i<argc-2; i++) {
		close(ifd[i]);
	}


	allocatedblocks_bf=(0xFFFFFFFF<<allocatedblocks)&0xFFFFFFFF;


	ibuf[PARAM_BLOCK_SIZE - PARAM_BLOCK_FOOTER_SIZE + 0] = (blocksize>>24) & 0xFF;
	ibuf[PARAM_BLOCK_SIZE - PARAM_BLOCK_FOOTER_SIZE + 1] = (blocksize>>16) & 0xFF;
	ibuf[PARAM_BLOCK_SIZE - PARAM_BLOCK_FOOTER_SIZE + 2] = (blocksize>>8)  & 0xFF;
	ibuf[PARAM_BLOCK_SIZE - PARAM_BLOCK_FOOTER_SIZE + 3] = (blocksize>>0)  & 0xFF;

	ibuf[PARAM_BLOCK_SIZE - PARAM_BLOCK_FOOTER_SIZE + 4] = (allocatedblocks_bf>>24) & 0xFF;
	ibuf[PARAM_BLOCK_SIZE - PARAM_BLOCK_FOOTER_SIZE + 5] = (allocatedblocks_bf>>16) & 0xFF;
	ibuf[PARAM_BLOCK_SIZE - PARAM_BLOCK_FOOTER_SIZE + 6] = (allocatedblocks_bf>>8)  & 0xFF;
	ibuf[PARAM_BLOCK_SIZE - PARAM_BLOCK_FOOTER_SIZE + 7] = (allocatedblocks_bf>>0)  & 0xFF;

	int ofd=open(argv[argc-1], O_WRONLY | O_CREAT | O_TRUNC,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (ofd<=0) {
		printf("ERROR: Unable to open file for writing: %s\n", argv[argc-1]);
		return -1;
	}
	printf("Writing to: %s\n", argv[argc-1]);
	write(ofd, ibuf, PARAM_BLOCK_SIZE);
	close(ofd);

	return 0;
}

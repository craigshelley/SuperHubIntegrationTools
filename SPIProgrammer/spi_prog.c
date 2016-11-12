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
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


int get(int fd, char * buf, int size) {
	int nread, ptr=0;
	while(ptr<size) {
			nread=read(fd, &buf[ptr], size-ptr);
			if (nread==0) break;
			ptr+=nread;
	}
	buf[ptr]='\0';

	return ptr;
}


int hexchar(char hexch) {
	int retval=-1;

	if (hexch>='0' && hexch<='9') {
		retval=hexch-'0'+0;
	} else if (hexch>='A' && hexch<='F') {
		retval=hexch-'A'+0xA;
	} else if (hexch>='a' && hexch<='f') {
		retval=hexch-'a'+0xa;
	}

	return retval;
}

unsigned int hex(char* hexstring) {
	unsigned int i, len;
	unsigned int retval=0;
	int chr;

	len=strlen(hexstring);

	for(i=0; i<len; i++) {
		chr=hexchar(hexstring[i]);
		if (chr<0) {
			retval=0;
		} else {
			retval<<=4;		
			retval+=chr;
		}
	}

	return retval;
}

int flash_read(int port, char * buf, int start, int len) {
	char query[16];

	printf("Reading data 0x%08x-0x%08x (0x%08x)...\n", start, start+len, len);


	sprintf(query, "R%c%c%c%c%c%c", (start>>16)&0xFF, (start>>8)&0xFF, (start>>0)&0xFF,  (len>>16)&0xFF, (len>>8)&0xFF, (len>>0)&0xFF);
	write(port, query, 7);
	if (get(port, buf, len)!=len) {
		printf("Timeout\n");
		return 4;
	}
	if (get(port, query, 5)!=5) {
		printf("Timeout\n");
		return 2;
	}
	if (strcmp(query, "Hello")!=0) {
		printf("Invalid response to read\n");
		return 3;
	}




	return 0;
}


int flash_write(int port, char * buf, int start, int len) {
	int written=0;
	char query[16];

	int er_start=start&0xFFFF0000;
	int er_end=(start+len-1)|0xFFFF;
	int er_ptr;

	printf("Erasing 0x%08x-0x%08x (0x%08x)...\n", er_start, er_end, er_end-er_start+1);
	for (er_ptr=er_start; er_ptr<er_end; er_ptr+=0x10000) {
		/*Enable writes, needs to be done on each loop*/
		write(port, "E", 1);
		if (get(port, query, 5)!=5) {
			printf("Timeout\n");
			return 2;
		}
		if (strcmp(query, "Hello")!=0) {
			printf("Invalid response to enable\n");
			return 3;
		}

		/*Sector erase*/
		sprintf(query, "S%c%c%c", (er_ptr>>16)&0xFF, (er_ptr>>8)&0xFF, (er_ptr>>0)&0xFF);
		write(port, query, 4);
		if (get(port, query, 5)!=5) {
			printf("Timeout\n");
			return 2;
		}
		if (strcmp(query, "Hello")!=0) {
			printf("Invalid response to sector erase\n");
			return 3;
		}
		
		/*Wait for completion*/
		char status=0;
		do {
			write(port, "Q", 1);
			if (get(port, query, 1+5)!=1+5) {
				printf("Timeout\n");
				return 0;
			}
			if (strcmp(&query[1], "Hello")!=0) {
				printf("Invalid response to status reg query\n");
				return 0;
			}
			status=query[0];
		} while((status & 0x01) !=0);

	


	}

	printf("Writing data 0x%08x-0x%08x (0x%08x)...\n", start, start+len, len);

	while (written<len) {
		/* Enable writes, needs to be done on each loop*/
		write(port, "E", 1);
		if (get(port, query, 5)!=5) {
			printf("Timeout\n");
			return 2;
		}
		if (strcmp(query, "Hello")!=0) {
			printf("Invalid response\n");
			return 3;
		}



		unsigned int towrite=len-written;
		if (towrite>256) towrite=256;
		sprintf(query, "P%c%c%c%c", ((start+written)>>16)&0xFF, ((start+written)>>8)&0xFF, ((start+written)>>0)&0xFF,  towrite&0xFF);
		write(port, query, 5);
		write(port, &buf[written], towrite);
		written+=towrite;

		if (get(port, query, 5)!=5) {
			printf("Timeout\n");
			return 2;
		}
		if (strcmp(query, "Hello")!=0) {
			printf("Invalid response\n");
			return 3;
		}
		
		char status=0;
		do {
			write(port, "Q", 1);
			if (get(port, query, 1+5)!=1+5) {
				printf("Timeout\n");
				return 0;
			}
			if (strcmp(&query[1], "Hello")!=0) {
				printf("Invalid response\n");
				return 0;
			}
			status=query[0];
		} while((status & 0x01) !=0);


	}

	/*Disable writes*/
	write(port, "D", 1);
	if (get(port, query, 5)!=5) {
		printf("Timeout\n");
		return 2;
	}
	if (strcmp(query, "Hello")!=0) {
		printf("Invalid response\n");
		return 3;
	}


	return 0;
}

int flash_bulkerase(int port) {
	char query[16];
	
	printf("Erasing device...\n");

	/*Enable writes, needs to be done on each loop*/
	write(port, "E", 1);
	if (get(port, query, 5)!=5) {
		printf("Timeout\n");
		return 2;
	}
	if (strcmp(query, "Hello")!=0) {
		printf("Invalid response to enable\n");
		return 3;
	}

	/*Sector erase*/
	write(port, "F", 1);
	if (get(port, query, 5)!=5) {
		printf("Timeout\n");
		return 2;
	}
	if (strcmp(query, "Hello")!=0) {
		printf("Invalid response to sector erase\n");
		return 3;
	}

	/*Wait for completion*/
	char status=0;
	do {
		write(port, "Q", 1);
		if (get(port, query, 1+5)!=1+5) {
			printf("Timeout\n");
			return 0;
		}
		if (strcmp(&query[1], "Hello")!=0) {
			printf("Invalid response to status reg query\n");
			return 0;
		}
		status=query[0];
	} while((status & 0x01) !=0);


	/*Disable writes*/
	write(port, "D", 1);
	if (get(port, query, 5)!=5) {
		printf("Timeout\n");
		return 2;
	}
	if (strcmp(query, "Hello")!=0) {
		printf("Invalid response\n");
		return 3;
	}

	return 0;

}

int main(int argc, char * argv[]) {

	int port, nread;
	unsigned char buf[512];
	struct termios term;
	int retval=0;

	if (argc<2) {
		printf("Usage:\n\n\t%s /dev/port [{-e}|{-r start_addr length output_file}|{-w start_addr input_file}]\n\n", argv[0]);
		printf("\t/dev/port     - Serial port\n");
		printf("\tstart_addr    - Starting address within EEPROM (Hexadecimal)\n");
		printf("\tlength        - Number of bytes to read (Hexadecimal)\n");
		printf("\toutput_file   - Filename to store data read from EEPROM\n");
		printf("\tinput_file    - Filename of data to be written to EEPROM\n\n");
		printf("\t-e            - Erase entire EEPROM\n");
		printf("\t-r            - Read data from EEPROM\n");
		printf("\t-w            - Write data to EEPROM\n\n");
		return 1;
	}

	
	port=open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY);

	if (!port) {
		printf("Unable to open port\n");
		return 1;
	}

	/*Force the read call to block while reading*/
	fcntl(port, F_SETFL, 0);


	tcgetattr(port, &term);

	/*Configure the serial port*/

	/*Set the Baud Rate*/
	cfsetospeed(&term, B115200);  /*Set output to 115200 baud*/
	cfsetispeed(&term, B115200);  /*Set input to 115200 baud*/

	/*POSIX Control Mode Flags*/
	term.c_cflag &= ~CSIZE;	    //Clear the old word size*
	term.c_cflag |= CS8;         //Set the new word size to 8 bits
	term.c_cflag &= ~CSTOPB;     //Set 1 stop bit
	term.c_cflag |= CREAD;       //Enable receiver
	term.c_cflag &= ~PARENB;     //Turn off parity
	term.c_cflag &= ~HUPCL;	    //Ignore Hang up
	term.c_cflag |= CLOCAL;	    //Local line - don't change port owner
	term.c_cflag &= ~CRTSCTS;    //Turn off flow control

	/*POSIX Local Mode Flags*/
	term.c_lflag &= ~ISIG;	    //Turn off signals
	term.c_lflag &= ~ICANON;	    //Turn off canonical input for raw mode
	term.c_lflag &= ~ECHO;	    //Turn off input echo
	term.c_lflag &= ~ECHOE;	    //Turn off erase characters
	term.c_lflag &= ~ECHOK;	    //Turn off echo NL after kill
	term.c_lflag &= ~ECHONL;	    //Turn off echo NL
	term.c_lflag |= NOFLSH;	    //Turn off flushing of input buffers
	term.c_lflag &= ~IEXTEN;	    //Turn off extended functions
	term.c_lflag &= ~ECHOCTL;    //Turn off echoing of control characters
	term.c_lflag &= ~ECHOKE;	    //Turn off BS-SP-BS entire line on line kill
	term.c_lflag &= ~FLUSHO;	    //Turn off output being flushed
	term.c_lflag &= ~TOSTOP;	    //Turn off send SIGTOU for background output

	/*POSIX Input Mode Flags*/
	term.c_iflag &= ~INPCK;	    //Turn off parity check
	term.c_iflag |= IGNPAR;	    //Ignore parity errors
	term.c_iflag &= ~PARMRK;	    //Turn off Mark parity errors
	term.c_iflag &= ~ISTRIP;	    //Turn off strip parity bits
	term.c_iflag &= ~IXON;	    //Turn off output software flow control
	term.c_iflag &= ~IXOFF;	    //Turn off input software flow control
	term.c_iflag &= ~IXANY;	    //Turn off allow any char to restart flow
	term.c_iflag |= IGNBRK;	    //Ignore break condition
	term.c_iflag &= ~BRKINT;	    //Turn off sending SIGINT when break detected
	term.c_iflag &= ~INLCR;	    //Turn off mapping NL to CR
	term.c_iflag &= ~IGNCR;	    //Turn off ignore CR
	term.c_iflag &= ~ICRNL;	    //Turn off mapping CR to NL
	term.c_iflag &= ~IUCLC;	    //Turn off map upercase to lowercase
	term.c_iflag &= ~IMAXBEL;    //Turn off echo BEL on input too long

	/*POSIX Output Mode Flags*/
	term.c_oflag &= ~OPOST;      //Turn off all output processing

	/*POSIX Control*/
	term.c_cc[VMIN] = 0;         //Use char timeouts
	term.c_cc[VTIME] = 10;   //Set timeout (deciseconds)
	tcsetattr(port, TCSANOW, &term);


	printf("Flushing buffers...\n");
	read(port, buf, 256);

	/*Reboot the device*/
	printf("Handshake..\n");
	write(port, "0", 1);
	if (get(port, buf, 5)!=5) {
		printf("Timeout\n");
		return 2;
	}
	if (strcmp(buf, "Hello")!=0) {
		printf("Invalid response\n");
		return 3;
	}


	/*Query the Device ID*/
	printf("Query ID..\n");
	write(port, "I", 1);
	if (get(port, buf, 3+5)!=3+5) {
		printf("Timeout\n");
		return 4;
	}
	if (strcmp(&buf[3], "Hello")!=0) {
		printf("Invalid response\n");
		return 5;
	}
	printf("Device ID 0x%02x%02x%02x\n", buf[0], buf[1], buf[2]);


	if (argc>2) {
		if (strcmp(argv[2], "-r")==0 && argc==6) {
			int start=hex(argv[3]);
			int len=hex(argv[4]);
			char * fbuf=malloc(len+1);
			if (fbuf==NULL) {
				return -1;
			}
			retval=flash_read(port, fbuf, start, len);
			int fd=open(argv[5], O_WRONLY | O_CREAT | O_TRUNC,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd<=0) {
				free(fbuf);
				return -1;
			}
			write(fd, fbuf, len);
			close(fd);
			free(fbuf);
		}

		if (strcmp(argv[2], "-w")==0 && argc==5) {
			int start=hex(argv[3]);
			struct stat stbuf;
			int fd=open(argv[4], O_RDONLY);
			if (fd<=0) {
				return -1;
			}
			fstat(fd, &stbuf);
			
			char * fbuf=malloc(stbuf.st_size);
			if (fbuf==NULL) {
				return -1;
			}
			read(fd, fbuf, stbuf.st_size);
			close(fd);

			retval=flash_write(port, fbuf, start, stbuf.st_size);
			free(fbuf);
		}


		if (strcmp(argv[2], "-e")==0 && argc==3) {
			retval=flash_bulkerase(port);
		}

	}


	close(port);


	return retval;
}

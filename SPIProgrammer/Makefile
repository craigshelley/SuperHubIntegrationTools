
all:	SPIProgrammer.hex spi_prog

program:
			pk2cmd -P -FSPIProgrammer.hex -M -R

spi_prog:               spi_prog.c
	                gcc spi_prog.c -o spi_prog

SPIProgrammer.hex:	SPIProgrammer.o
			gplink -m -o SPIProgrammer.hex SPIProgrammer.o

SPIProgrammer.o:	SPIProgrammer.asm config.inc macros.inc
			gpasm -c SPIProgrammer.asm

.PHONY: clean
clean:			
			rm -f *.hex *.o *.lst *.cod *.map spi_prog

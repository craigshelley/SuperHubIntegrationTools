;    This program is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with this program.  If not, see <http://www.gnu.org/licenses/>.


;*************************************************
;Includes
;*************************************************
	INCLUDE	config.inc
	INCLUDE macros.inc

;*************************************************
;Configuration word
;*************************************************
	__CONFIG _CP_OFF & _CCP1_RB0 & _DEBUG_OFF & _WRT_PROTECT_ALL & _CPD_OFF & _LVP_OFF & _BODEN_OFF & _MCLR_ON & _PWRTE_ON & _WDT_OFF & _HS_OSC


;*************************************************
;Variables
;*************************************************
	udata

DATA_COUNT                      res 3
COMMAND                         res 1
SPI_BUF                         res 1


DELAY1                          res 1
DELAY2                          res 1
DELAY3                          res 1

;*************************************************
;Reset and Interrupt Vectors
;*************************************************
rstvect	code	0x0000
	goto	init

;*************************************************
;Start of Program
;*************************************************
watercontrol	code


init	
	_bank 1
	clrf	ANSEL
	movlw	(1<<B_SI) | (1<<B_RX) 
	movwf	TRISB
	_bank 0

	clrf	DATA_COUNT
	
	movlw	(1<<B_CS) | (1<<B_HOLD)
	movwf	PORTB

	;Set up serial port
	_bank 1
	movlw	(1<<TXEN) | (1<<BRGH)
	movwf	TXSTA
	movlw	10 ; 10=115200 baud with 20Mhz osc
	movwf	SPBRG
	_bank 0

	movlw	(1<<SPEN) | (1<<CREN) | (1<<SREN)
	movwf	RCSTA



loop:
	movlw	'H'
	call	serial_send
	movlw	'e'
	call	serial_send
	movlw	'l'
	call	serial_send
	movlw	'l'
	call	serial_send
	movlw	'o'
	call	serial_send
	

	call	serial_receive
	movwf	COMMAND

	movlw	'I'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	call	flash_readid

	movlw	'R'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	call	flash_readdata

	movlw	'E'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	call	flash_writeenable

	movlw	'D'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	call	flash_writedisable

	movlw	'P'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	call	flash_pageprogram

	movlw	'S'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	call	flash_sectorerase

	movlw	'Q'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	call	flash_readstatusreg

	movlw	'F'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	call	flash_bulkerase

	movlw	'B'
	subwf	COMMAND, w
	btfsc	STATUS, Z
	goto	0

	goto	loop



flash_readdata:
	bcf	PORTB, B_CS
	movlw	0x03
	call	spi_io

	call	serial_receive
	call	spi_io

	call	serial_receive
	call	spi_io

	call	serial_receive
	call	spi_io

	call	serial_receive
	movwf	DATA_COUNT+2
	call	serial_receive
	movwf	DATA_COUNT+1
	call	serial_receive
	movwf	DATA_COUNT+0

flash_dataread_loop:
	movf	DATA_COUNT+0, f
	btfss	STATUS, Z
	goto	flash_dataread_dec0
	movf	DATA_COUNT+1, f
	btfss	STATUS, Z
	goto	flash_dataread_dec1
	movf	DATA_COUNT+2, f
	btfss	STATUS, Z
	goto	flash_dataread_dec2

	goto	flash_dataread_return

	
flash_dataread_dec2:
	decf	DATA_COUNT+2, f
flash_dataread_dec1:
	decf	DATA_COUNT+1, f
flash_dataread_dec0:
	decf	DATA_COUNT+0, f


	movlw	0x00
	call	spi_io
	call	serial_send
	goto	flash_dataread_loop

flash_dataread_return:

	bsf	PORTB, B_CS
	return


flash_readid:
	bcf	PORTB, B_CS
	movlw	0x9F
	call	spi_io
	movlw	0x00
	call	spi_io
	call	serial_send
	movlw	0x00
	call	spi_io
	call	serial_send
	movlw	0x00
	call	spi_io
	call	serial_send
	bsf	PORTB, B_CS
	return

flash_readstatusreg:
	bcf	PORTB, B_CS
	movlw	0x05
	call	spi_io
	movlw	0x00
	call	spi_io
	call	serial_send
	bsf	PORTB, B_CS
	return


flash_writeenable:
	bsf	PORTB, B_W
	bcf	PORTB, B_CS
	movlw	0x06
	call	spi_io
	bsf	PORTB, B_CS
	return

flash_writedisable:
	bcf	PORTB, B_CS
	movlw	0x04
	call	spi_io
	bsf	PORTB, B_CS
	bcf	PORTB, B_W
	return

flash_bulkerase:
	bcf	PORTB, B_CS
	movlw	0xC7
	call	spi_io
	bsf	PORTB, B_CS
	return


flash_pageprogram:
	bcf	PORTB, B_CS
	movlw	0x02
	call	spi_io

	;Address MSB
	call	serial_receive
	call	spi_io

	;Address
	call	serial_receive
	call	spi_io

	;Address LSB
	call	serial_receive
	call	spi_io

	;Number of bytes 1-256 i.e.0=256
	call	serial_receive
	movwf	DATA_COUNT

flash_pageprogram_loop:
	;Data bytes
	call	serial_receive
	call	spi_io

	decfsz	DATA_COUNT, f
	goto	flash_pageprogram_loop

	bsf	PORTB, B_CS
	return

flash_sectorerase:
	bcf	PORTB, B_CS
	movlw	0xD8
	call	spi_io

	;Address MSB
	call	serial_receive
	call	spi_io

	;Address
	call	serial_receive
	call	spi_io

	;Address LSB
	call	serial_receive
	call	spi_io

	bsf	PORTB, B_CS
	return



spi_io:
	movwf	SPI_BUF


	movfw	SPI_BUF
	bcf	SPI_BUF, 7
	btfsc	PORTB, B_SI
	bsf	SPI_BUF, 7
	andlw	(1<<7)
	btfss	STATUS, Z
	goto	$+3
	bcf	PORTB, B_SO
	goto	$+2
	bsf	PORTB, B_SO
	bsf	PORTB, B_SCK
	bcf	PORTB, B_SCK

	movfw	SPI_BUF
	bcf	SPI_BUF, 6
	btfsc	PORTB, B_SI
	bsf	SPI_BUF, 6
	andlw	(1<<6)
	btfss	STATUS, Z
	goto	$+3
	bcf	PORTB, B_SO
	goto	$+2
	bsf	PORTB, B_SO
	bsf	PORTB, B_SCK
	bcf	PORTB, B_SCK

	movfw	SPI_BUF
	bcf	SPI_BUF, 5
	btfsc	PORTB, B_SI
	bsf	SPI_BUF, 5
	andlw	(1<<5)
	btfss	STATUS, Z
	goto	$+3
	bcf	PORTB, B_SO
	goto	$+2
	bsf	PORTB, B_SO
	bsf	PORTB, B_SCK
	bcf	PORTB, B_SCK

	movfw	SPI_BUF
	bcf	SPI_BUF, 4
	btfsc	PORTB, B_SI
	bsf	SPI_BUF, 4
	andlw	(1<<4)
	btfss	STATUS, Z
	goto	$+3
	bcf	PORTB, B_SO
	goto	$+2
	bsf	PORTB, B_SO
	bsf	PORTB, B_SCK
	bcf	PORTB, B_SCK

	movfw	SPI_BUF
	bcf	SPI_BUF, 3
	btfsc	PORTB, B_SI
	bsf	SPI_BUF, 3
	andlw	(1<<3)
	btfss	STATUS, Z
	goto	$+3
	bcf	PORTB, B_SO
	goto	$+2
	bsf	PORTB, B_SO
	bsf	PORTB, B_SCK
	bcf	PORTB, B_SCK

	movfw	SPI_BUF
	bcf	SPI_BUF, 2
	btfsc	PORTB, B_SI
	bsf	SPI_BUF, 2
	andlw	(1<<2)
	btfss	STATUS, Z
	goto	$+3
	bcf	PORTB, B_SO
	goto	$+2
	bsf	PORTB, B_SO
	bsf	PORTB, B_SCK
	bcf	PORTB, B_SCK

	movfw	SPI_BUF
	bcf	SPI_BUF, 1
	btfsc	PORTB, B_SI
	bsf	SPI_BUF, 1
	andlw	(1<<1)
	btfss	STATUS, Z
	goto	$+3
	bcf	PORTB, B_SO
	goto	$+2
	bsf	PORTB, B_SO
	bsf	PORTB, B_SCK
	bcf	PORTB, B_SCK

	movfw	SPI_BUF
	bcf	SPI_BUF, 0
	btfsc	PORTB, B_SI
	bsf	SPI_BUF, 0
	andlw	(1<<0)
	btfss	STATUS, Z
	goto	$+3
	bcf	PORTB, B_SO
	goto	$+2
	bsf	PORTB, B_SO
	bsf	PORTB, B_SCK
	bcf	PORTB, B_SCK


	movfw	SPI_BUF

	return




serial_send:
	;Wait for room in buffer
	_bank	1
	btfss	TXSTA, TRMT
	goto	$-1
	_bank	0

	movwf	TXREG
	return

serial_receive:
	;Wait for data to arrive
	btfss	PIR1, RCIF
	goto	$-1

	movfw	RCREG
	return;

serial_check:
	btfsc	PIR1, RCIF
	retlw	0xFF
	retlw	0x00

delay1s:
	clrf	DELAY1
	clrf	DELAY2
	movlw	20
	movwf	DELAY3

	decfsz	DELAY1, f
	goto	$-1
	decfsz	DELAY2, f
	goto	$-3
	decfsz	DELAY3, f
	goto	$-5


	return
	

	END

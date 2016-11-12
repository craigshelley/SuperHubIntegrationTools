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
;Configuration
;*************************************************

;*************************************************
;Assembler Options
;*************************************************

	;Bank bits warning message suppression
	ERRORLEVEL -302

	;Processor Type
	PROCESSOR 16f88
	INCLUDE p16f88.inc

	;Default Base for literals
	RADIX dec


;*************************************************
;Hardware Abstraction Table 
;*************************************************
B_SI            equ 0         ;Pin 6
B_SO            equ 1         ;Pin 7
B_RX            equ 2         ;Pin 8  (COMMS RX)
B_W             equ 3         ;Pin 9
B_HOLD          equ 4         ;Pin 10
B_TX            equ 5         ;Pin 11 (COMMS TX)
B_CS            equ 6         ;Pin 12
B_SCK           equ 7         ;Pin 13

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
;Macro Definitions
;*************************************************

;Register bank selection
_bank macro bankno
	;Configure RP0
	if	((bankno & 1) != 0)
	bsf	STATUS, RP0
	else
	bcf	STATUS, RP0
	endif
	endm

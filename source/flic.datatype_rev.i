VERSION		EQU	1
REVISION	EQU	9

DATE	MACRO
		dc.b '27.4.2009'
		ENDM

VERS	MACRO
		dc.b 'flic.datatype 1.9'
		ENDM

VSTRING	MACRO
		dc.b 'flic.datatype 1.9 (27.4.2009)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: flic.datatype 1.9 (27.4.2009)',0
		ENDM

VERSION = 1
REVISION = 9

.macro DATE
.ascii "27.4.2009"
.endm

.macro VERS
.ascii "flic.datatype 1.9"
.endm

.macro VSTRING
.ascii "flic.datatype 1.9 (27.4.2009)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: flic.datatype 1.9 (27.4.2009)"
.byte 0
.endm

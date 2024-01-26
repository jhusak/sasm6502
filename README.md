# sasm6502
## A simple interactive assembler with atari Action! language code inserts output.

If in need to insert 6502 code snipped to your Action code, you may find this tool useful.
It is based on "assembler" function taken from atari800 emulator code, with labels added.

### Usage

Invoke sasm6502 with optional assembly address (default $2000):
```
$>sasm6502 0x3000
```
A prompt will show:
```
$>sasm6502 
Simple 6502 assembler for Action! by Jakub Husak (enter empty line to exit)
2000: 
```

Then you may code:

```
Simple 6502 assembler for Action! by Jakub Husak (enter empty line to exit)
2000: ldx #0
2002: lda VAL
2005: sta ADDR,x
2008: inx
2009: bne $2005
200B:
```

and get output:

```
[
 $A2 $00	; LDX #0
 $AD VAL	; LDA VAL
 $9D ADDR	; STA ADDR,X
 $E8	; INX
 $D0 $FA	; BNE $2005
]
```
If you want labels in 0-page addressing, add 0 before label:
```
lda 0SAVMSC
```

### Building

Type "make" to build. Apropriate gcc needed.

### Bugs

Sometimes when entering asm a mistake was made, the sasm state is not fully recovered and sometimes asm instruction entered is wrongly assembled. So be warned :) Will be fixed in spare time.

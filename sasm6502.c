/*
 * sasm6502.c - simple asm6502 cut off from atari800 emulator by jhusak
 * Date: 01.2024
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2010 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define _POSIX_C_SOURCE 200112L /* for snprintf */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#ifdef GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif

#ifdef MONITOR_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <stdarg.h>

#define UWORD	unsigned short
#define SWORD	short
#define UBYTE	unsigned char
#define MEMORY_dPutByte(a,b) mem[a]=(b)
#define MEMORY_dPutWord(a,b) {mem[a]=b&0xff; mem[a+1]=(b)>>8;};
#define MEMORY_SafeGetByte(a) (mem[a])

#include "sasm6502.h"
#include "util.h"

UBYTE mem[0x10000];
char comments[0x10000*128];
char labels[0x10000*128];

static const char instr6502[256][10] = {
	"BRK", "ORA (1,X)", "CIM", "ASO (1,X)", "NOP 1", "ORA 1", "ASL 1", "ASO 1",
	"PHP", "ORA #1", "ASL", "ANC #1", "NOP 2", "ORA 2", "ASL 2", "ASO 2",

	"BPL 0", "ORA (1),Y", "CIM", "ASO (1),Y", "NOP 1,X", "ORA 1,X", "ASL 1,X", "ASO 1,X",
	"CLC", "ORA 2,Y", "NOP !", "ASO 2,Y", "NOP 2,X", "ORA 2,X", "ASL 2,X", "ASO 2,X",

	"JSR 2", "AND (1,X)", "CIM", "RLA (1,X)", "BIT 1", "AND 1", "ROL 1", "RLA 1",
	"PLP", "AND #1", "ROL", "ANC #1", "BIT 2", "AND 2", "ROL 2", "RLA 2",

	"BMI 0", "AND (1),Y", "CIM", "RLA (1),Y", "NOP 1,X", "AND 1,X", "ROL 1,X", "RLA 1,X",
	"SEC", "AND 2,Y", "NOP !", "RLA 2,Y", "NOP 2,X", "AND 2,X", "ROL 2,X", "RLA 2,X",


	"RTI", "EOR (1,X)", "CIM", "LSE (1,X)", "NOP 1", "EOR 1", "LSR 1", "LSE 1",
	"PHA", "EOR #1", "LSR", "ALR #1", "JMP 2", "EOR 2", "LSR 2", "LSE 2",

	"BVC 0", "EOR (1),Y", "CIM", "LSE (1),Y", "NOP 1,X", "EOR 1,X", "LSR 1,X", "LSE 1,X",
	"CLI", "EOR 2,Y", "NOP !", "LSE 2,Y", "NOP 2,X", "EOR 2,X", "LSR 2,X", "LSE 2,X",

	"RTS", "ADC (1,X)", "CIM", "RRA (1,X)", "NOP 1", "ADC 1", "ROR 1", "RRA 1",
	"PLA", "ADC #1", "ROR", "ARR #1", "JMP (2)", "ADC 2", "ROR 2", "RRA 2",

	"BVS 0", "ADC (1),Y", "CIM", "RRA (1),Y", "NOP 1,X", "ADC 1,X", "ROR 1,X", "RRA 1,X",
	"SEI", "ADC 2,Y", "NOP !", "RRA 2,Y", "NOP 2,X", "ADC 2,X", "ROR 2,X", "RRA 2,X",


	"NOP #1", "STA (1,X)", "NOP #1", "SAX (1,X)", "STY 1", "STA 1", "STX 1", "SAX 1",
	"DEY", "NOP #1", "TXA", "ANE #1", "STY 2", "STA 2", "STX 2", "SAX 2",

	"BCC 0", "STA (1),Y", "CIM", "SHA (1),Y", "STY 1,X", "STA 1,X", "STX 1,Y", "SAX 1,Y",
	"TYA", "STA 2,Y", "TXS", "SHS 2,Y", "SHY 2,X", "STA 2,X", "SHX 2,Y", "SHA 2,Y",

	"LDY #1", "LDA (1,X)", "LDX #1", "LAX (1,X)", "LDY 1", "LDA 1", "LDX 1", "LAX 1",
	"TAY", "LDA #1", "TAX", "ANX #1", "LDY 2", "LDA 2", "LDX 2", "LAX 2",

	"BCS 0", "LDA (1),Y", "CIM", "LAX (1),Y", "LDY 1,X", "LDA 1,X", "LDX 1,Y", "LAX 1,X",
	"CLV", "LDA 2,Y", "TSX", "LAS 2,Y", "LDY 2,X", "LDA 2,X", "LDX 2,Y", "LAX 2,Y",


	"CPY #1", "CMP (1,X)", "NOP #1", "DCM (1,X)", "CPY 1", "CMP 1", "DEC 1", "DCM 1",
	"INY", "CMP #1", "DEX", "SBX #1", "CPY 2", "CMP 2", "DEC 2", "DCM 2",

	"BNE 0", "CMP (1),Y", "ESCRTS #1", "DCM (1),Y", "NOP 1,X", "CMP 1,X", "DEC 1,X", "DCM 1,X",
	"CLD", "CMP 2,Y", "NOP !", "DCM 2,Y", "NOP 2,X", "CMP 2,X", "DEC 2,X", "DCM 2,X",


	"CPX #1", "SBC (1,X)", "NOP #1", "INS (1,X)", "CPX 1", "SBC 1", "INC 1", "INS 1",
	"INX", "SBC #1", "NOP", "SBC #1 !", "CPX 2", "SBC 2", "INC 2", "INS 2",

	"BEQ 0", "SBC (1),Y", "ESCAPE #1", "INS (1),Y", "NOP 1,X", "SBC 1,X", "INC 1,X", "INS 1,X",
	"SED", "SBC 2,Y", "NOP !", "INS 2,Y", "NOP 2,X", "SBC 2,X", "INC 2,X", "INS 2,X"
};

/* Opcode type:
   bits 1-0 = instruction length
   bit 2    = instruction reads from memory (without stack-manipulating instructions)
   bit 3    = instruction writes to memory (without stack-manipulating instructions)
   bits 7-4 = adressing type:
     0 = NONE (implicit)
     1 = ABSOLUTE
     2 = ZPAGE
     3 = ABSOLUTE_X
     4 = ABSOLUTE_Y
     5 = INDIRECT_X
     6 = INDIRECT_Y
     7 = ZPAGE_X
     8 = ZPAGE_Y
     9 = RELATIVE
     A = IMMEDIATE
     B = STACK 2 (RTS)
     C = STACK 3 (RTI)
     D = INDIRECT (JMP () )
     E = ESCRTS
     F = ESCAPE */
const UBYTE MONITOR_optype6502[256] = {
	0x01, 0x56, 0x01, 0x5e, 0x22, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x13, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0x01, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0x13, 0x56, 0x01, 0x5e, 0x26, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x17, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0x01, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0xc1, 0x56, 0x01, 0x5e, 0x22, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x13, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0x01, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0xb1, 0x56, 0x01, 0x5e, 0x22, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0xd3, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0x01, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0xa2, 0x5a, 0x01, 0x5a, 0x2a, 0x2a, 0x2a, 0x2a, 0x01, 0xa2, 0x01, 0xa2, 0x1b, 0x1b, 0x1b, 0x1b,
	0x92, 0x6a, 0x01, 0x6a, 0x7a, 0x7a, 0x8a, 0x8a, 0x01, 0x4b, 0x01, 0x4b, 0x3b, 0x3b, 0x4b, 0x4b,
	0xa2, 0x56, 0xa2, 0x56, 0x26, 0x26, 0x26, 0x26, 0x01, 0xa2, 0x01, 0xa2, 0x17, 0x17, 0x17, 0x17,
	0x92, 0x66, 0x01, 0x66, 0x76, 0x76, 0x86, 0x86, 0x01, 0x47, 0x01, 0x47, 0x37, 0x37, 0x47, 0x47,
	0xa2, 0x56, 0xa2, 0x5e, 0x26, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x17, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0xe2, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0xa2, 0x56, 0xa2, 0x5e, 0x26, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x17, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0xf2, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f
};

/* Parses S in search for a hexadecimal number. On success stores the number
   in HEXVAL and returns TRUE; otherwise returns FALSE. */
static int real_parse_hex(const char *s, UWORD *hexval)
{
	if (s[0]=='\0') return FALSE;
	int x;

	if (s[0]=='$') x = Util_sscanhex(s+1);
	else x = Util_sscandec(s);

	if (x < 0 || x > 0xffff)
		return FALSE;
	*hexval = (UWORD) x;
	return TRUE;
}

static void safe_gets(char *buffer, size_t size, char const *prompt)
{
	fputs(prompt, stdout);
	if (fgets(buffer, size, stdin) == NULL)
		buffer[0] = 0;
	Util_chomp(buffer);
}


static UWORD assembler(UWORD addr)
{
	fprintf(stderr,"Simple assembler (enter empty line to exit)\n");
	memset(mem,0,sizeof(mem));
	memset(comments,0,sizeof(comments));
	memset(labels,0,sizeof(labels));

	for (;;) {
		char s[128];  /* input string */
		char c[128];  /* converted input */
		char *sp;     /* input pointer */
		char *cp;     /* converted input pointer */
		char *vp;     /* value pointer (the value is stored in s) */
		char *tp;     /* type pointer (points at type character '0', '1' or '2' in converted input) */
		int i;
		int isa;      /* the operand is "A" */
		UWORD value = 0;

		char prompt[7];
		prompt[0]='\0';
		//snprintf(prompt, sizeof(prompt), "%04X: ", (int) addr);
		fprintf(stderr, "%04X: ", (int) addr);
		safe_gets(s, sizeof(s), prompt);
		if (s[0] == '\0')
			return addr;

		Util_strupper(s);
		strcpy(&comments[addr*128],s); // buffer will be passed to comment

		sp = s;
		cp = c;
		/* copy first three characters */
		for (i = 0; i < 3 && *sp != '\0'; i++)
			*cp++ = *sp++;
		/* insert space before operands */
		*cp++ = ' ';

		tp = NULL;
		isa = FALSE;

		/* convert input to format of instr6502[] table */
		while (*sp != '\0') {
			switch (*sp) {
			case ' ':
			case '\t':
				sp++;
				break;
			case '#':
			case '(':
			case ')':
			case ',':
				isa = FALSE;
				*cp++ = *sp++;
				break;
			default:
				if (tp != NULL) {
					if (*sp == 'X' || *sp == 'Y') {
						*cp++ = *sp++;
						break;
					}
					goto invalid_instr;
				}
				vp = s;
				do
					*vp++ = *sp++;
				while (strchr(" \t$@#(),", *sp) == NULL && *sp != '\0');
				/* If *sp=='\0', strchr() should return non-NULL,
				   but we do an extra check to be on safe side. */
				*vp++ = '\0';
				tp = cp++;
				*tp = '0';
				isa = (s[0] == 'A' && s[1] == '\0');
				break;
			}
		}
		if (cp[-1] == ' ')
			cp--;    /* no arguments (e.g. NOP or ASL @) */
		*cp = '\0';

		/* if there's an operand, get its value */
		if (tp != NULL && !real_parse_hex(s, &value)) {
			strcpy(&labels[addr*128],s); // set label for this address
			//printf("Invallid operand! (%s)\n",s);
			//continue;
		}

		for (;;) {
			/* search table for instruction */
			for (i = 0; i < 256; i++) {
				if (strcmp(instr6502[i], c) == 0) {
					if (tp == NULL) {
						mem[addr++]=i;
					}
					else if (*tp == '0') {
						if (labels[addr*128]!='\0') {
							printf("Labels not allowed in branches\n");
						}
						else {
							value -= (addr + 2);
							if ((SWORD) value < -128 || (SWORD) value > 127)
								printf("Branch out of range (give destination address as argument)\n");
							else {
								mem[addr++]=i;
								mem[addr++]=value;
							}
						}
					}
					else if (*tp == '1') {
						c[3] = '\0';
						if (isa && (strcmp(c, "ASL") == 0 || strcmp(c, "LSR") == 0 ||
						            strcmp(c, "ROL") == 0 || strcmp(c, "ROR") == 0)) {
							printf("\"%s A\" is ambiguous.\n"
							       "Use \"%s\" for accumulator mode or \"%s 0A\" for zeropage mode.\n", c, c, c);
						}
						else {
							mem[addr++]=i;
							mem[addr++]=value;
						}
					}
					else { /* *tp == '2' */
						mem[addr++]=i;
						mem[addr++]=value;
						mem[addr++]=value>>8;
					}
					goto next_instr;
				}
			}
			/* not found */
			if (tp == NULL || *tp == '2')
				break;

			++*tp;
			char * l=&labels[addr*128];
			if (*l!='\0') {
				if ( *tp=='1') {if (*l!='0') *tp = '2';}
				if ( *tp=='2') {if (*l=='0')
					{
						char b[128];
						char * t=strstr(&comments[addr*128],l);
						if (t) *t=' '; // hacky remove 0

						strcpy(b,&l[1]); // remove 0 by shifting
						strcpy(l,b);
					}
				}
			}
			else if (*tp == '1' && value > 0xff)
				*tp = '2';
		}
	invalid_instr:
		printf("Invalid instruction!\n");
	next_instr:
		;
	}
}

int main(int argc, char ** argv)
{

	int astart;

	if (argc==1)
		astart=0x2000;
	else
		astart=strtol(argv[1],NULL,0);
	UWORD addr = assembler(astart);
	printf ("[\n");
	int t=-1;
	for (int i=astart; i<addr; i++) {
		if (comments[i*128]) {
			if (t>=0) { printf("\t; %s\n",&comments[t*128]); t=-1; }
			t=i;
		}
		printf(" $%02X", mem[i]);

		if (labels[i*128]) {
			int t=labels[i*128]=='0';
			printf(" %s",&labels[i*128+t]);
			i+= t? 1 : 2;
		}
	}
	if (t>=0) printf("\t; %s\n",&comments[t*128]);
	printf("]\n");
}

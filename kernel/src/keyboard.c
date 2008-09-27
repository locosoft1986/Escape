/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/keyboard.h"
#include "../h/util.h"
#include "../h/video.h"

#define KBD_DATA_PORT	0x60

#define	A_NPRINT		'^'

#define K_ERR			0x00
#define K_ESC			0x01
#define K_BACKSP		0x02
#define K_LCTRL			0x03
#define K_RCTRL			0x04
#define K_LALT			0x05
#define K_RALT			0x06
#define K_PRINT			0x07
#define K_LSHIFT		0x08
#define K_RSHIFT		0x09
#define K_CAPSLOCK		0x0A
#define K_F1			0x0B
#define K_F2			0x0C
#define K_F3			0x0D
#define K_F4			0x0E
#define K_F5			0x0F
#define K_F6			0x10
#define K_F7			0x11
#define K_F8			0x12
#define K_F9			0x13
#define K_F10			0x14
#define K_F11			0x15
#define K_F12			0x16
#define K_NUMTGL		0x17
#define K_SCROLL		0x18

/* "generic 105-key PC" keyboard of my notebook :) */
struct {
	u8 special;
	u8 plain;
	u8 shift;
	u8 altGr;
} germanKeys[] = {
	/* 0x00 */	{K_ERR,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x01 */	{K_ESC,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x02 */	{K_PRINT,	'1',		'!',		A_NPRINT},
	/* 0x03 */	{K_PRINT,	'2',		'"',		A_NPRINT},
	/* 0x04 */	{K_PRINT,	'3',		A_NPRINT,	A_NPRINT},
	/* 0x05 */	{K_PRINT,	'4',		'$',		A_NPRINT},
	/* 0x06 */	{K_PRINT,	'5',		'%',		A_NPRINT},
	/* 0x07 */	{K_PRINT,	'6',		'&',		A_NPRINT},
	/* 0x08 */	{K_PRINT,	'7',		'/',		A_NPRINT},
	/* 0x09 */	{K_PRINT,	'8',		'(',		A_NPRINT},
	/* 0x0A */	{K_PRINT,	'9',		')',		A_NPRINT},
	/* 0x0B */	{K_PRINT,	'0',		'=',		A_NPRINT},
	/* 0x0C */	{K_PRINT,	A_NPRINT,	'?',		'\\'},	/* szlig not supported yet */
	/* 0x0D */	{K_PRINT,	'\'',		'`',		A_NPRINT},
	/* 0x0E */	{K_BACKSP,	A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x0F */	{K_PRINT,	'\t',		'\t',		'\t'},
	/* 0x10 */	{K_PRINT,	'q',		'Q',		'@'},
	/* 0x11 */	{K_PRINT,	'w',		'W',		A_NPRINT},
	/* 0x12 */	{K_PRINT,	'e',		'E',		A_NPRINT},
	/* 0x13 */	{K_PRINT,	'r',		'R',		A_NPRINT},
	/* 0x14 */	{K_PRINT,	't',		'T',		A_NPRINT},
	/* 0x15 */	{K_PRINT,	'z',		'Z',		A_NPRINT},
	/* 0x16 */	{K_PRINT,	'u',		'U',		A_NPRINT},
	/* 0x17 */	{K_PRINT,	'i',		'I',		A_NPRINT},
	/* 0x18 */	{K_PRINT,	'o',		'O',		A_NPRINT},
	/* 0x19 */	{K_PRINT,	'p',		'P',		A_NPRINT},
	/* 0x1A */	{K_PRINT,	A_NPRINT,	A_NPRINT,	A_NPRINT},	/* uuml not supported */
	/* 0x1B */	{K_PRINT,	'+',		'*',		'~'},
	/* 0x1C */	{K_PRINT,	'\n',		'\n',		'\n'},
	/* 0x1D */	{K_LCTRL,	A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x1E */	{K_PRINT,	'a',		'A',		A_NPRINT},
	/* 0x1F */	{K_PRINT,	's',		'S',		A_NPRINT},
	/* 0x20 */	{K_PRINT,	'd',		'D',		A_NPRINT},
	/* 0x21 */	{K_PRINT,	'f',		'F',		A_NPRINT},
	/* 0x22 */	{K_PRINT,	'g',		'G',		A_NPRINT},
	/* 0x23 */	{K_PRINT,	'h',		'H',		A_NPRINT},
	/* 0x24 */	{K_PRINT,	'j',		'J',		A_NPRINT},
	/* 0x25 */	{K_PRINT,	'k',		'K',		A_NPRINT},
	/* 0x26 */	{K_PRINT,	'l',		'L',		A_NPRINT},
	/* 0x27 */	{K_PRINT,	A_NPRINT,	A_NPRINT,	A_NPRINT},	/* ouml not supported */
	/* 0x28 */	{K_PRINT,	A_NPRINT,	A_NPRINT,	A_NPRINT},	/* auml not supported */
	/* 0x2B */	{K_PRINT,	'<',		'>',		'|'},		/* ?? */
	/* 0x2A */	{K_LSHIFT,	A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x29 */	{K_PRINT,	'#',		'\'',		A_NPRINT},
	/* 0x2C */	{K_PRINT,	'y',		'Y',		A_NPRINT},
	/* 0x2D */	{K_PRINT,	'x',		'X',		A_NPRINT},
	/* 0x2E */	{K_PRINT,	'c',		'C',		A_NPRINT},
	/* 0x2F */	{K_PRINT,	'v',		'V',		A_NPRINT},
	/* 0x30 */	{K_PRINT,	'b',		'B',		A_NPRINT},
	/* 0x31 */	{K_PRINT,	'n',		'N',		A_NPRINT},
	/* 0x32 */	{K_PRINT,	'm',		'M',		A_NPRINT},
	/* 0x33 */	{K_PRINT,	',',		';',		A_NPRINT},
	/* 0x34 */	{K_PRINT,	'.',		':',		A_NPRINT},
	/* 0x35 */	{K_PRINT,	'-',		'_',		A_NPRINT},
	/* 0x36 */	{K_RSHIFT,	A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x37 */	{K_PRINT,	'*',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x38 */	{K_LALT,	A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x39 */	{K_PRINT,	' ',		' ',		' '},
	/* 0x3A */	{K_CAPSLOCK,A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x3B */	{K_F1,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x3C */	{K_F2,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x3D */	{K_F3,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x3E */	{K_F4,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x3F */	{K_F5,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x40 */	{K_F6,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x41 */	{K_F7,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x42 */	{K_F8,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x43 */	{K_F9,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x44 */	{K_F10,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x45 */	{K_NUMTGL,	A_NPRINT,	A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x46 */	{K_SCROLL,	A_NPRINT,	A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x47 */	{K_PRINT,	'7',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x48 */	{K_PRINT,	'8',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x49 */	{K_PRINT,	'9',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x4A */	{K_PRINT,	'-',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x4B */	{K_PRINT,	'4',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x4C */	{K_PRINT,	'5',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x4D */	{K_PRINT,	'6',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x4E */	{K_PRINT,	'+',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x4F */	{K_PRINT,	'1',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x50 */	{K_PRINT,	'2',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x51 */	{K_PRINT,	'3',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x52 */	{K_PRINT,	'0',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x53 */	{K_PRINT,	',',		A_NPRINT,	A_NPRINT},	/* keypad */
	/* 0x54 */	{K_ERR,		A_NPRINT,	A_NPRINT,	A_NPRINT},	/* ?? */
	/* 0x55 */	{K_ERR,		A_NPRINT,	A_NPRINT,	A_NPRINT},	/* ?? */
	/* 0x56 */	{K_PRINT,	'<',		'>',		'|'},
	/* 0x57 */	{K_F11,		A_NPRINT,	A_NPRINT,	A_NPRINT},
	/* 0x58 */	{K_F12,		A_NPRINT,	A_NPRINT,	A_NPRINT},
};

/* modifier */
bool shiftDown = false;
bool altGrDown = false;

/*

2b (\|), on a 102-key keyboard

2c (Z), 2d (X), 2e (C), 2f (V), 30 (B), 31 (N), 32 (M), 33 (,<), 34 (.>), 35 (/?), 36 (RShift)

37 (Keypad-*) or (* /PrtScn) on a 83/84-key keyboard

38 (LAlt), 39 (Space bar),

3a (CapsLock)

3b (F1), 3c (F2), 3d (F3), 3e (F4), 3f (F5), 40 (F6), 41 (F7), 42 (F8), 43 (F9), 44 (F10)

45 (NumLock)

46 (ScrollLock)

47 (Keypad-7/Home), 48 (Keypad-8/Up), 49 (Keypad-9/PgUp)

4a (Keypad--)

4b (Keypad-4/Left), 4c (Keypad-5), 4d (Keypad-6/Right), 4e (Keypad-+)

4f (Keypad-1/End), 50 (Keypad-2/Down), 51 (Keypad-3/PgDn)

52 (Keypad-0/Ins), 53 (Keypad-./Del)

54 (Alt-SysRq) on a 84+ key keyboard */

#define SCAN_LSHIFT				0x2A
#define SCAN_RSHIFT				0x36
#define SCAN_SEQ_INIT			0xE0
#define SCAN_SEQ_NOBR_INIT		0xE1
#define IS_BREAK(code) (code & 0x80)

#define KBD_BUF_SIZE			10

u8 keyBuffer[KBD_BUF_SIZE];
u8 keyBufferPos = 0;

void kbd_handleIntrpt(void) {
	u8 scanCode = inb(KBD_DATA_PORT);
	bool isBrk = IS_BREAK(scanCode);
	u8 key = 0;
	switch(scanCode) {
		case SCAN_SEQ_INIT:
			/* ignore */
			break;
		
		case SCAN_LSHIFT:
		case SCAN_RSHIFT:
			shiftDown = !isBrk;
			break;
		
		default:
			if(scanCode < sizeof(germanKeys) / sizeof(germanKeys[0])) {
				if(shiftDown) {
					key = germanKeys[scanCode].shift;
				}
				else if(altGrDown) {
					key = germanKeys[scanCode].altGr;
				}
				else {
					key = germanKeys[scanCode].plain;
				}
			}
			break;
	}
	
	/* store into buffer */
	if(key && key != A_NPRINT/* && keyBufferPos < KBD_BUF_SIZE*/) {
		/*keyBuffer[keyBufferPos++] = key;*/
		/*vid_printf("%c",key);*/
	}
	
	vid_printf("ScanCode=%x\n",scanCode);
	/* acknowledge scancode */
	outb(0x20,0x20);
}

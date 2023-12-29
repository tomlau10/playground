#ifndef _CONIOEXT_H	/* ver 1.05 */
#define _CONIOEXT_H

/* Function list begin */
#ifdef _WIN32
	/* Windows unique functions and header files */
	#include <conio.h>
	#include <windows.h>
	static inline void textattr(int attr);		// Set text attribute
#else
	/* Linux unique functions and header files */
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <termios.h>
	#include <fcntl.h>
	typedef struct { int X, Y; } COORD;			// Coordinate type same as Windows
	static inline int conMode(int setMode);		// Set console mode
	static inline int ungetch(int ch);			// Same as Windows ungetch()
	static inline int kbhit();					// Return true if keyboard is pressed { conMode(1) is needed before use }
	static inline void textattr(char *attr);	// Set text attribute	
#endif
static inline void conResize(int h, int w);		// Resize console
static inline void conTitle(char *title);		// Set console title
static inline void showCursor(int bShow);		// Set console cursor visibilty
static inline void gotoxy(int x, int y);		// Move console cursor to (x,y) { upper left corner is (0,0) }
static inline COORD CURSOR();					// Return current console cursor location { (0,0) based }
static inline void clrscr();					// Clear screen and move cursor to home coordinate
static inline int getkey();						// Get a character key code
/* Function list end */

/**************************************************************************************************************/

/* Implenmentation begin */
#ifdef _WIN32
	/* Windows implementation */
	
	// Text attributes constants
	#define FG_RED		FOREGROUND_RED
	#define FG_GREEN	FOREGROUND_GREEN
	#define FG_BLUE		FOREGROUND_BLUE
	#define FG_YELLOW	FOREGROUND_RED|FOREGROUND_GREEN
	#define FG_MAGENTA	FOREGROUND_RED|FOREGROUND_BLUE
	#define FG_CYAN		FOREGROUND_GREEN|FOREGROUND_BLUE
	#define FG_WHITE	FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE
	#define FG_INTENSE	FOREGROUND_INTENSITY
	#define BG_RED		BACKGROUND_RED
	#define BG_GREEN	BACKGROUND_GREEN
	#define BG_BLUE		BACKGROUND_BLUE
	#define BG_YELLOW	BACKGROUND_RED|BACKGROUND_GREEN
	#define BG_MAGENTA	BACKGROUND_RED|BACKGROUND_BLUE
	#define BG_CYAN		BACKGROUND_GREEN|BACKGROUND_BLUE
	#define BG_WHITE	BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE
	#define BG_INTENSE	BACKGROUND_INTENSITY
	
	enum {	// Special key codes in windows
		KEY_TAB='\t', KEY_ENTER='\r', KEY_ESC='\e', KEY_SPACE=' ',
		KEY_BACKSPAC='\b', KEY_HOME=327, KEY_UP=328, KEY_PGUP=329,
		KEY_LEFT=331, KEY_RIGHT=333, KEY_END=335, KEY_DOWN=336,
		KEY_PGDN=337, KEY_INSERT=338, KEY_DEL=339
	};
	
	static inline void conResize(int h, int w) {
		char cmd[50];
		sprintf(cmd, "MODE con:cols=%d lines=%d", w, h);
		system(cmd);
	}
	static inline void conTitle(char *title) { SetConsoleTitle(title); }
	static inline void showCursor(int bShow) {
		CONSOLE_CURSOR_INFO	ci;
		ci.dwSize = 20; ci.bVisible = bShow;
		SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
	}
	static inline void gotoxy(int x, int y) {
		COORD pos = {x, y};
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
	}
	static inline COORD CURSOR() {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		return csbi.dwCursorPosition;
	}
	static inline void textattr(int attr) {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
	}
	static inline void clrscr() { system("cls"); }
	static inline int getkey() {
		int ch = getch();
		if (!ch || ch==224) ch = 256+getch();
		return ch;
	}
#else
	/* Linux implementation */
	
	// Text attributes constants
	#define FGBG_RESET	"\e[0m"
	#define FG_INTENSE	"\e[1m"
	#define FG_NORMAL	"\e[22m"
	#define FG_BLACK	"\e[30m"
	#define FG_RED		"\e[31m"
	#define FG_GREEN	"\e[32m"
	#define FG_YELLOW	"\e[33m"
	#define FG_BLUE		"\e[34m"
	#define FG_MAGENTA	"\e[35m"
	#define FG_CYAN		"\e[36m"
	#define FG_WHITE	"\e[37m"
	#define	FG_RESET	"\e[39m"
	#define BG_BLACK	"\e[40m"
	#define BG_RED		"\e[41m"
	#define BG_GREEN	"\e[42m"
	#define BG_YELLOW	"\e[43m"
	#define BG_BLUE		"\e[44m"
	#define BG_MAGENTA	"\e[45m"
	#define BG_CYAN		"\e[46m"
	#define BG_WHITE	"\e[47m"
	#define	BG_RESET	"\e[49m"
	
	enum {	// Special key codes in linux
		KEY_TAB='\t', KEY_ENTER='\n', KEY_ESC='\e', KEY_SPACE=' ',
		KEY_BACKSPAC=127, KEY_END=432, KEY_HOME=434,
		KEY_INSERT=550, KEY_DEL=551, KEY_PGUP=553, KEY_PGDN=554,
		KEY_UP=439, KEY_DOWN=440, KEY_RIGHT=441, KEY_LEFT=442
	};
	
	static inline int conMode(int setMode) {
		/* conMode(): Set console mode
		 *  0 = standard mode with line buffer and screen echo
		 *  1 = turned off line buffer and screen echo, for reading keypress
		 *  2 = internal use for temporary turning off line buffer and screen echo
		 * -1 = internal use for returning current mode
		 * Advised to insert:
		 * conMode(1) on program start if needed, and
		 * conMode(0) on program exit
		 * Note: if mode is set to 1, set conMode(0) before any standard scanf() */
		static struct termios oldt, newt;
		static int inited = 0, mode = 0;
		if (setMode == -1) return mode;		// return current mode
		if (!inited) {
			inited = 1;
			tcgetattr(0, &oldt);
			newt = oldt;
			oldt.c_lflag |= (ICANON | ECHO);
			newt.c_lflag &= ~(ICANON | ECHO);
		}
		if (setMode) tcsetattr(0, TCSANOW, &newt);
		else tcsetattr(0, TCSANOW, &oldt);
		mode = setMode;
		return 0;	// this return value is useless
	}
	static inline void conResize(int h, int w) { printf("\e[8;%d;%dt", h, w); }
	static inline void conTitle(char *title) { printf("\e]0;%s\007", title); }
	static inline void showCursor(int bShow) { printf("\e[?25%c", bShow?'h':'l'); }
	static inline void gotoxy(int x, int y) { printf("\e[%d;%df", y+1, x+1); }
	static inline COORD CURSOR() {
		int x, y;
		if (!conMode(-1)) conMode(2);
		printf("\e[6n");
		scanf("\e[%d;%dR", &y, &x);
		COORD pos = {x-1, y-1};
		if (conMode(-1) == 2) conMode(0);
		return pos;
	}
	static inline void textattr(char *attr) { printf("%s", attr); }
	static inline void clrscr() { printf("\e[2J\e[;f"); }
	static inline int ungetch(int ch) { return ungetc(ch, stdin); }
	static inline int kbhit() {
		static int inited = 0, oldf, newf;
		register int ch;
		if (!conMode(-1)) {
			// conMode(1) is needed because setting a "temporary on" mode will have screen echo bugs
			fprintf(stderr, "Error: kbhit() must be used with conMode(1).\n");
			exit(1);
		}
		if (!inited) {
			inited = 1;
			oldf = fcntl(0, F_GETFL, 0);
			newf = oldf;
			oldf &= ~O_NONBLOCK;
			newf |= O_NONBLOCK;
		}
		fcntl(0, F_SETFL, newf);
		ch = getchar();
		fcntl(0, F_SETFL, oldf);
		if (ch != EOF) return ungetc(ch, stdin);
		return 0;
	}
	static inline int getkey() {
		int ch;
		if (!conMode(-1)) conMode(2);
		ch = getchar();
		if (kbhit() && ch=='\e') {
			ch += 256;
			while (kbhit()) ch += getchar();
		}
		if (conMode(-1) == 2) conMode(0);
		return ch;
	}
#endif
/* Implenmentation end */

#endif	/* _CONIOEXT_H */

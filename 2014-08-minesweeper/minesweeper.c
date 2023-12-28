#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "conioext.h"

#ifndef _WIN32
	#define clock() getmstime()		// Linux clock() function has bugs
	int getmstime () {
		struct timespec tp;
		clock_gettime(CLOCK_MONOTONIC, &tp);
		return tp.tv_sec*1000 + (int)(tp.tv_nsec/1.0e6);
	}
#endif

/* game settings */
#define TITLE		"MINESWEEPER"
#define VERSION		"v1.01"
#define AUTHOR		"Tom Lau"
#define	WIN_HEIGHT	25
#define WIN_WIDTH	63

// Text atrribute settings
#ifdef _WIN32
	#define ATTR_RED	FG_RED|FG_INTENSE
	#define ATTR_GREEN	FG_GREEN|FG_INTENSE
	#define ATTR_RESET	FG_WHITE
#else
	#define ATTR_RED	FG_RED FG_BOLD
	#define ATTR_GREEN	FG_GREEN
	#define ATTR_RESET	FGBG_RESET
#endif

enum {	// difficulty preset
	col_e=9, row_e=9, mines_e=10, safe_e=1,
	col_m=16, row_m=16, mines_m=40, safe_m=1,
	col_h=30, row_h=16, mines_h=99, safe_h=2,
};

// grid state
enum { HIDDEN, OPENED, FLAGGED };
// flag mode 
enum { EXPAND, OPEN, FLAG };
// game state
enum { PREPARE, RUNNING, LOST, WON };

// global var
char grid[row_h][col_h], state[row_h][col_h];
COORD mine[mines_h], cursor;
int gameStatus, gridsLeft, flagMode, flags, col, row, mines, safe;
int startTime, currentTime, timeElapsed;

/* general functions */
int abs(int n) { return n>0?n:-n; }

/* hud functions */
int isValid(r, c) {
	if (r<0||r>=row) return 0;
	if (c<0||c>=col) return 0;
	return 1;
}
void drawElement(int r, int c) {
	char ch;
	if (state[r][c]==FLAGGED) ch = 'x';
	else if (state[r][c]==HIDDEN) ch = '.';
	else if (grid[r][c] == '0') ch = ' ';
	else ch = grid[r][c];
	if (ch=='x' || ch=='*') textattr(ATTR_RED);
	else if (ch!='.') textattr(ATTR_GREEN);
	gotoxy(c*2+2, r+1);
	putchar(ch);
	textattr(ATTR_RESET);
}
void drawCursor() {
	if (flagMode!=FLAG) textattr(ATTR_GREEN);
	else textattr(ATTR_RED);
	gotoxy(cursor.X*2+1, cursor.Y+1);
	putchar('[');
	gotoxy(cursor.X*2+3, cursor.Y+1);
	putchar(']');
	textattr(ATTR_RESET);
}
void drawStatus() {
	gotoxy(0, row+2);
	printf("Mines Left: %3d | Grids Left: %3d | Time: %.1fs", mines-flags, gridsLeft, (float)timeElapsed/1000);
}
void drawGrid() {
	int i,j;
	showCursor(0);
	clrscr();
	putchar('+');
	for (j=0; j<=col*2; j++) putchar('-');
	printf("+\n");
	for (i=0; i<row; i++) {
		gotoxy(0, i+1);
		printf("| ");
		for (j=0; j<col; j++) drawElement(i, j);
		printf(" |\n");
	}
	gotoxy(0, i+1);
	putchar('+');
	for (j=0; j<=col*2; j++) putchar('-');
	printf("+\n");
	drawCursor();
	drawStatus();
}
void updateTimer() {
	currentTime = clock();
	if (currentTime-startTime > timeElapsed) {
		timeElapsed+=100;
		drawStatus();
	}
}

/* user command functions */
int getInput() {
	drawStatus();
	
	#ifndef _WIN32
		conMode(1);
		while (!kbhit()) { updateTimer(); usleep(1000); }
		conMode(0);
	#else
		while (!kbhit()) { updateTimer(); Sleep(1); }	// update timer
	#endif
	
	return toupper(getkey());	// get input
}
void moveCursor(int dir) {
	COORD oldpos = cursor;
	switch (dir) {
		case KEY_UP: case 'W': cursor.Y--; break;
		case KEY_LEFT: case 'A': cursor.X--; break;
		case KEY_RIGHT: case 'D': cursor.X++; break;
		case KEY_DOWN: case 'S': cursor.Y++; break;
		default: return;
	}
	if (!isValid(cursor.Y, cursor.X)) {cursor = oldpos;}
	else {	// refresh cursor
		gotoxy(oldpos.X*2+1, oldpos.Y+1);
		putchar(' ');
		gotoxy(oldpos.X*2+3, oldpos.Y+1);
		putchar(' ');
		drawCursor();
	}
}
void openrc(int r, int c, int mode) {
	int i, j, cnt=0;
	if (state[r][c]==FLAGGED && mode!=FLAG) return;
	if (state[r][c]==OPENED && mode==EXPAND) return;
	if (state[r][c]!=OPENED && mode==FLAG) {	// toggle flag
		state[r][c] = state[r][c]==HIDDEN?FLAGGED:HIDDEN;
		flags += state[r][c]==FLAGGED?1:-1;
		drawElement(r, c);
		return;
	}
	if (grid[r][c] == '*') {	// opened a mine
		gameStatus = LOST;
		for (cnt=0; cnt<mines; cnt++) {
			i = mine[cnt].Y;
			j = mine[cnt].X;
			state[i][j] = OPENED;
		}
		return;
	}
	if (state[r][c]==OPENED) {
		if (grid[r][c] == '0') return;
		for (i=r-1; i<=r+1; i++) for (j=c-1; j<=c+1; j++) {
			if (!isValid(i, j)) continue;
			if (r==i && c==j) continue;
			if (state[i][j]==FLAGGED) cnt++;
		}
	} else if (state[r][c]==HIDDEN) {	// open grid(r,c)
		state[r][c] = OPENED;
		gridsLeft--;
		drawElement(r, c);
	}
	if (cnt+'0' == grid[r][c]) {	// open neighbouring grids if putting enough flags
		for (i=r-1; i<=r+1; i++) for (j=c-1; j<=c+1; j++) {
			if (!isValid(i, j)) continue;
			if (i==r && j==c) continue;
			if (state[i][j]==HIDDEN) openrc(i, j, EXPAND);
		}
	}
}

/* initialize */
void init() {
	srand(time(NULL));
	conResize(WIN_HEIGHT, WIN_WIDTH);
	conTitle(TITLE);
}
void genMines() {
	int i,j,r,c,cnt;
	for (cnt=0; cnt<mines; cnt++) {
		do {
			i = rand()%row;
			j = rand()%col;
		} while (grid[i][j]=='*' || (abs(i-cursor.Y)<=safe && abs(j-cursor.X)<=safe));
		grid[i][j] = '*';
		mine[cnt].Y = i;
		mine[cnt].X = j;
		for (r=i-1; r<=i+1; r++) for (c=j-1; c<=j+1; c++) {
			if (!isValid(r, c)) continue;
			if (c==j && r==i) continue;
			if (grid[r][c] == '*') continue;
			grid[r][c]++;	// increase neighbouring indicators
		}
	}
}
int gameInit(int restart) {
	int ch, valid=0;
	if (!restart) {
		showCursor(1);
		clrscr();
		printf("+===============================+\n");
		printf("#          Minesweeper          #\n");
		printf("+===============================+\n");
		printf("| 1. Easy     %2d * %2d  %d mines |\n", row_e, col_e, mines_e);
		printf("| 2. Medium   %2d * %2d  %d mines |\n", row_m, col_m, mines_m);
		printf("| 3. Hard     %2d * %2d  %d mines |\n", row_h, col_h, mines_h);
		printf("| 0. Exit                       |\n");
		printf("+===============================+\n");
		printf("Choice?");
		do {
			ch = getkey();
			valid = 1;
			switch (ch) {
				case '1': row=row_e; col=col_e; mines=mines_e; safe=safe_e; break;
				case '2': row=row_m; col=col_m; mines=mines_m; safe=safe_m; break;
				case '3': row=row_h; col=col_h; mines=mines_h; safe=safe_h; break;
				case '0': return 0;
				default: valid = 0;
			}
		} while (!valid);
	}
	
	int i,j;
	for (i=0; i<row; i++) for (j=0; j<col; j++) {
		grid[i][j] = '0';
		state[i][j] = HIDDEN;
	}
	cursor.X = cursor.Y = 0;
	flags = timeElapsed = 0;
	gameStatus = PREPARE;
	flagMode = OPEN;
	gridsLeft = col*row-mines;
	
	drawGrid();
	gotoxy(0, row+4);
	puts("Instructions:\nArrow Keys\tMove Cursor");
	puts("SPACE\t\tSelect");
	puts("X\t\tToggle Flag Mode");
	printf("ESC\t\tExit to Main Menu");
	ch = 0;
	while (ch!=KEY_SPACE && ch!=KEY_ESC) moveCursor(ch=toupper(getkey()));
	ungetch(ch);
	genMines();	// generate mines on open
	startTime = currentTime = clock();	// start timer
	gameStatus = RUNNING;
	return 1;
}

/* main program */
int main() {
	init();
	int ch=0;
	while (gameInit(ch==KEY_ENTER)) {
		while (gameStatus==RUNNING) {
			ch = getInput();
			switch (ch) {
				case KEY_SPACE: openrc(cursor.Y, cursor.X, flagMode); break;
				case 'X': flagMode = flagMode==OPEN?FLAG:OPEN; drawCursor(); break;
				default: moveCursor(ch);
			}
			if (gameStatus==RUNNING && !gridsLeft) gameStatus = WON;
			if (ch == KEY_ESC) break;
		}
		if (gameStatus!=RUNNING) {	// end game
			drawGrid();
			gotoxy(0, row+3);
			if (gameStatus==WON) {
				textattr(ATTR_GREEN);
				puts("             W   W   IIIII   N   N");
				puts("             W   W     I     NN  N");
				puts("             W W W     I     N N N");
				puts("             W W W     I     N  NN");
				puts("              W W    IIIII   N   N");
			} else if (gameStatus==LOST){
				textattr(ATTR_RED);
				puts("        BBBB     OOO    M   M   BBBB");
				puts("        B   B   O   O   MM MM   B   B");
				puts("        BBBB    O   O   M M M   BBBB");
				puts("        B   B   O   O   M   M   B   B");
				puts("        BBBB     OOO    M   M   BBBB");
			}
			textattr(ATTR_RESET);
			printf("Press ENTER to restart or ESC to quit .....");
			do { ch=getkey(); } while (ch!=KEY_ESC && ch!=KEY_ENTER);
		}
		showCursor(1);
	}
	clrscr();
	return 0;
}
#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

/* game settings */
#define TITLE		"S N A K E"
#define VERSION		"v1.02"
#define AUTHOR		"Tom Lau"
#define	HEIGHT		25
#define WIDTH		40
#define WIN_HEIGHT	31
#define WIN_WIDTH	79
#define MAX_LV		25
#define MAX_CLEVER	90
#define MAX_SLEN	25
#define MAX_MONEY	3

/* score config */
#define SCORE_MONEY	10
#define SCORE_ITEM	100
#define SCORE_KILL	500
#define LVUP_MONEY	3

/* item effect */
#define TURN_FREEZE	100
#define TURN_REVIVE 250
#define EFF_STUPID	3
#define EFF_CUT		2
#define PROB_TOTAL	100

/* record type */
#define DATA		"score"
#define MAX_MODE	2
#define MAX_REC		5
#define MAX_NAME	20
#define DEF_NAME	"Player"
typedef struct {
	char name[21];
	int score, lv_start, lv, clever, slen, elpsTime;
} record;

/* constants */
enum {	// things
	EMPTY, PLAYER, SHEAD, SNAKE, BLOCK, MONEY,
	FREEZE, STUPID, CUT, KILL,	// help items
	MAX_THING					// things count
};
const char chr_t[MAX_THING] = {	// representing chars
	' ', 'I', 'S', '+', '#', '$',
	'F', 'L', 'C', 'K'
};
const int prob_t[MAX_THING] = {	// item probability
	0,0,0,0,0,0,
	40, 25, 25, 10				// should add up to 100
};
enum {	// special key code
	ENTER=13, ESC=27, SPACE=32,
	UP=256+72, LEFT=256+75, RIGHT=256+77, DOWN=256+80
};

/* global variables */
int screen[WIDTH][HEIGHT], prob[PROB_TOTAL];
int mode, cmd, eaten, goldTaken, killed, freezeTurns, interval;
int lastTime, currTime, moveTime;		// used by timer functions
COORD player, snake[MAX_SLEN];			// player vs snake :o)
record hsrec[MAX_MODE][MAX_REC], rec;	// highscore records
int recNum[MAX_MODE];
HANDLE handle;	// used by console screen functions

/* console screen functions */
COORD cursor() {	// return console cursor position
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handle, &csbi);
	return csbi.dwCursorPosition;
}
void gotoxy(int x, int y) {	// set console cursor position
	COORD pos = {x, y};
	SetConsoleCursorPosition(handle, pos);
}
void setCursorVisibility(BOOL visibility) {	// set console cursor visibility
	CONSOLE_CURSOR_INFO	ci;
	ci.dwSize = 20; ci.bVisible = visibility;
	SetConsoleCursorInfo(handle, &ci);
}
void clrline(int y) {
	DWORD temp;
	COORD scrn = {0, y};
	FillConsoleOutputAttribute(handle, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE, WIN_WIDTH, scrn, &temp);
	FillConsoleOutputCharacter(handle, ' ', WIN_WIDTH, scrn, &temp);
	gotoxy(0, y);
}
void clrscr() {
	int i;
	for (i=0; i<WIN_HEIGHT; i++) {
		clrline(i);
		Sleep(15);	// animation effect
	}
	gotoxy(0,0);
}
void printHR(int y) {
	DWORD temp;
	COORD scrn = {0, y};
	FillConsoleOutputCharacter(handle, '-', WIN_WIDTH, scrn, &temp);
	gotoxy(0, y+1);
}

/* general functions */
void delay(int ms) {
	int endTime = clock()+ms;
	while (clock()<endTime && !kbhit()) Sleep(1);
}
void flash(char *msg, int x, int y) {
	int i,len;
	for (len=0; msg[len]; len++);
	do {
		for (gotoxy(x, y),i=0; i<=len; i++) {
			if (i>0) {
				gotoxy(x+i-1,y);
				putchar(msg[i-1]);
			}
			if (i<len) putchar(' ');
			delay(50);
			if (kbhit()) break;
		}
	} while (!kbhit());
	gotoxy(x,y);
	printf("%s", msg);
}
int getkey() {
	int ch = getch();
	if (!ch || ch==224) ch = 256 + getch();
	return ch;
}
int random(int min, int max) {
	return min + rand()%(max-min+1);
}
int sign(int n) {
	if (n>0) return 1;
	if (n<0) return -1;
	return 0;
}

/* highscore functions */
void clrRec(record *rec) {
	strcpy(rec->name, DEF_NAME);
	rec->score = rec->lv_start= rec->lv = 0;
	rec->clever = rec->slen = rec->elpsTime = 0;
}
int compRec(record a, record b, int m) {
// return -1 if a is better; 0 if equal; 1 if b is better
	if (!m) {
		if (a.score>b.score) return -1;
		else if (b.score>a.score) return 1;
		else if (a.lv>b.lv) return -1;
		else if (b.lv>a.lv) return 1;
	} else {
		if (a.elpsTime>b.elpsTime) return -1;
		else if (b.elpsTime>a.elpsTime) return 1;
		else if (a.score>b.score) return -1;
		else if (b.score>a.score) return 1;
	}
	return strcmp(a.name, b.name);
}
void readData() {
	FILE *f = fopen(DATA, "r");
	if (!f) return;
	int invalid=0, i, m;
	char buf;
	record tmp;
	recNum[0] = recNum[1] = 0;
	while (!feof(f) && !invalid) {
		clrRec(&tmp);
		for (i=0; i<MAX_NAME+1; i++) {
			fscanf(f, "%c", &buf);
			if (buf==',' || i>=MAX_NAME) break;
			else tmp.name[i] = buf;
		}
		if (feof(f)) break;
		if (buf!=',' || !tmp.name[0]) { invalid=1; break; }
		else tmp.name[i]=0;
		fscanf(f, "%d,%d,%d,%d", &m, &tmp.score, &tmp.lv_start, &tmp.lv);
		fscanf(f, ",%d,%d,%d%*c", &tmp.clever, &tmp.slen, &tmp.elpsTime);
		if (m<0 || m>=MAX_MODE) invalid=1;
		else if (recNum[m]>=MAX_REC) invalid=1;
		else if (tmp.lv_start<1 || tmp.lv_start>MAX_LV) invalid=1;
		else if (tmp.lv<tmp.lv_start || tmp.lv>MAX_LV) invalid=1;
		else if (tmp.clever<0 || tmp.clever>MAX_CLEVER) invalid=1;
		else if (tmp.slen<0 || tmp.slen>MAX_SLEN) invalid=1;
		else if (tmp.elpsTime<=0) invalid=1;
		else if (recNum[m]>0) invalid=compRec(hsrec[m][recNum[m]-1],tmp,m)>0;
		if (!invalid) hsrec[m][recNum[m]++] = tmp;
	}
	fclose(f);
	if (invalid) recNum[0] = recNum[1] = 0;
}
void writeData() {
	FILE *f = fopen(DATA, "w");
	if (!f) return;
	int i,m;
	for (m=0; m<MAX_MODE; m++) for (i=0; i<recNum[m]; i++) {
		fprintf(f, "%s,", hsrec[m][i].name);
		fprintf(f, "%d,%d,%d,%d", m, hsrec[m][i].score, hsrec[m][i].lv_start, hsrec[m][i].lv);
		fprintf(f, ",%d,%d,%d\n", hsrec[m][i].clever, hsrec[m][i].slen, hsrec[m][i].elpsTime);
	}
	fclose(f);
}
void printData() {
	const char* s1 = " No. Name                  Score  Level  S.Cleverness\tS.Length  Time Elapsed";
	int i,j;
	clrscr();
	puts("\n >>>>> TOP 5 HIGHSCORE <<<<<");
	printHR(cursor().Y);
	for (j=0; j<2; j++) {
		puts("");
		switch(j) {
			case 0: puts(" Standard Mode\t\t>>>>> Sort By Score"); break;
			case 1: puts(" Suvival Mode\t\t>>>>> Sort By Time Elapsed"); break;
		}
		puts(s1);
		printHR(cursor().Y);
		for (i=0; i<5; i++) {
			printf("  %d  ", i+1);
			if (i<recNum[j]) {
				printf("%s\t", hsrec[j][i].name);
				gotoxy(27,cursor().Y);
				printf("%05d  %02d-%02d\t", hsrec[j][i].score, hsrec[j][i].lv_start, hsrec[j][i].lv);
				printf("%13d\t%8d  %11.1fs\n", hsrec[j][i].clever, hsrec[j][i].slen, (float)hsrec[j][i].elpsTime/1000);
			} else puts("");
		}
		printHR(cursor().Y);
	}
	do {
		flash("Press ENTER to continue...", 0,cursor().Y);
	} while (getkey() != ENTER);
}
int insertData() {
	int rank, i;
	for (rank=0; rank<recNum[mode]; rank++)
		if (compRec(hsrec[mode][rank], rec, mode)>0) break;
	if (rank>=MAX_REC) return 0;
	for (i=MAX_REC-1; i>rank; i--) hsrec[mode][i] = hsrec[mode][i-1];
	hsrec[mode][rank] = rec;
	if (recNum[mode]<MAX_REC) recNum[mode]++;
	writeData();
	return 1;
}

/* spot functions */
int validSpot(COORD pos) {
	return (pos.X>=0&&pos.X<WIDTH) && (pos.Y>=0&&pos.Y<HEIGHT);
}
int checkSpot(COORD pos) {
	if (!validSpot(pos)) return -1;
	return screen[pos.X][pos.Y];
}
void markSpot(COORD pos, int thing) {
	WORD attr = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
	switch (thing) {
		case SHEAD: case SNAKE: attr = FOREGROUND_RED|FOREGROUND_INTENSITY; break;
		case PLAYER: attr = FOREGROUND_GREEN|FOREGROUND_INTENSITY; break;
		case EMPTY: case BLOCK: break;
		case MONEY: attr = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY; break;
		default: attr = FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY; break;
		
	}
	COORD scrn = {pos.X*2, pos.Y};
	DWORD temp;
	FillConsoleOutputAttribute(handle, attr, 1, scrn, &temp);
	FillConsoleOutputCharacter(handle, chr_t[thing], 1, scrn, &temp);
	screen[pos.X][pos.Y] = thing;
}
void removeSpot(COORD pos) {
	markSpot(pos, EMPTY);
}
COORD randSpot() {
	COORD pos;
	do {
		pos.X = random(0, WIDTH-1);
		pos.Y = random(0, HEIGHT-1);
	} while (checkSpot(pos)!=EMPTY);
	return pos;
}
COORD nearSpot(COORD oldpos, int allowPLAYER) {
	COORD pos;
	int cnt=0, radius=0;
	do {
		cnt++;
		pos = oldpos;
		switch (random(0, 3)) {
			case 0: pos.X -= 1+radius; break;
			case 1: pos.X += 1+radius; break;
			case 2: pos.Y -= 1+radius; break;
			case 3: pos.Y += 1+radius; break;
		}
		if (checkSpot(pos)==PLAYER && allowPLAYER) break;	// eat player
		if (!(cnt%100)) radius++;	// increase search radius
	} while (checkSpot(pos)!=EMPTY);
	return pos;
}

/* hud functions */
void updateStatus(int line, char* msg) {
	gotoxy(0, HEIGHT+line);
	switch (line) {
		case 1:
			printf("Level: %2d | $%05d", rec.lv, rec.score);
			printf(" | S.Cleverness: %2d%% | S.Lenth: %2d", rec.clever, rec.slen);
			printf(" | Time Elapsed: %.1fs", (float)rec.elpsTime/1000);
			break;
		default:
			if (!msg[0] || line==2) {
				clrline(HEIGHT+line);
				gotoxy(0, HEIGHT+line);
			}
			printf("%s", msg);
			break;
	}
}
void updateTimer() {
	currTime = clock();
	if (currTime-lastTime >= 100) {	// update on 100ms
		lastTime = currTime;
		rec.elpsTime += 100;
		gotoxy(69, HEIGHT+1);	// timer location
		printf("%.1fs", (float)rec.elpsTime/1000);
	}
}
void countDown(int sec) {
	char msg[30];
	while (sec--) {
		sprintf(msg, "The game will start in %ds", sec+1);
		updateStatus(3, msg);
		Sleep(500);
		removeSpot(player);
		Sleep(500);
		markSpot(player, PLAYER);
	}
	updateStatus(2, "Game Start!");
	updateStatus(3, "");
}

/* things functions */
int randThing() {
	return prob[random(0,PROB_TOTAL-1)];
}
void takeThing(int thing) {
	int i;
	switch (thing) {
		case MONEY:
			markSpot(randSpot(), MONEY);
			goldTaken++;
			if (!(goldTaken%LVUP_MONEY)) {
				if (rec.lv<MAX_LV) {
					rec.lv++;
					interval = 500/rec.lv;
					markSpot(randSpot(), BLOCK);
				}
				if (rec.slen<MAX_SLEN && !killed) {
					updateStatus(2,"The snake is LENGTHENED!");
					rec.slen++;
				}
				markSpot(randSpot(), randThing());	// random a help item
			}
			if (rec.clever<MAX_CLEVER) rec.clever++;
			break;
		case FREEZE:
			if (killed) break;
			freezeTurns = TURN_FREEZE;
			break;
		case STUPID:
			rec.clever -= EFF_STUPID;
			if (rec.clever<0) rec.clever=0;
			updateStatus(2, "The snake is LESS CLEVER!");
			break;
		case CUT: case KILL:
			if (killed) break;
			if (rec.slen-EFF_CUT<=0 || thing==KILL) {
				for (i=0; i<rec.slen; i++) removeSpot(snake[i]);
				rec.slen = 0;
				killed = 1;
				freezeTurns = TURN_REVIVE;
				updateStatus(2, "The snake is KILLED!");
				rec.score += SCORE_KILL;
			} else {
				for (i=rec.slen-EFF_CUT; i<rec.slen; i++)
					removeSpot(snake[i]);
				rec.slen -= EFF_CUT;
				updateStatus(2, "The snake is CUT!");
			}
			break;
	}
	if (thing==MONEY) rec.score += SCORE_MONEY*rec.lv_start + rec.lv*3 + rec.clever*4 + rec.slen*2;
	else rec.score += SCORE_ITEM;
	updateStatus(1,"");
}

/* initialize */
void changeName() {
	char buf[100];
	clrscr();
	puts("\n >>>>> ENTER YOUR NAME <<<<<");
	printHR(cursor().Y);
	puts("\n Challenger who are you?");
	printf(" If you do not tell, you will be called as '%s'.\n", rec.name);
	printf(" MAXIMUM %d characters.\n\n", MAX_NAME);
	printHR(cursor().Y);
	setCursorVisibility(TRUE);
	printf(" Your answer: ");
	gets(buf);
	if (buf[0]) strncpy(rec.name, buf, MAX_NAME);
	setCursorVisibility(FALSE);
}
void init() {
	srand(time(NULL));
	timeBeginPeriod(1);
	char buf[50];
	sprintf(buf, "MODE con:cols=%d lines=%d", WIN_WIDTH, WIN_HEIGHT);
	system(buf);
	sprintf(buf, "Title %s", TITLE);
	system(buf);
	handle = GetStdHandle(STD_OUTPUT_HANDLE);
	setCursorVisibility(FALSE);
	readData();
	clrRec(&rec);
	
	int i, j, thing;
	// initialize the prob array
	for (i=0,thing=FREEZE; thing<MAX_THING; thing++)
		for (j=0; j<prob_t[thing]; j++) prob[i++] = thing;

	// welcome screen
	printHR(10); puts("");
	puts("     SSSSSS      N    AAA    K   K   EEEEE");
	puts("     S       NNN N   AA AA   K KKK   E    ");
	puts("     SSSSS   N N N   A   A   KKK     EEEEE");
	puts("         S   N NNN   AAAAA   K KKK   E    ");
	puts("    SSSSSS   N       A   A   K   K   EEEEE\n");
	printHR(cursor().Y);
	do {
		flash("Press ENTER to continue...", 40,25);
	} while (getkey() != ENTER);
	changeName();
}
void placeObjects() {
	int i;
	markSpot(player=randSpot(), PLAYER);
	markSpot(snake[0]=randSpot(), SHEAD);
	for (i=1; i<rec.slen; i++) markSpot(snake[i]=nearSpot(snake[i-1],0), SNAKE);
	for (i=0; i<rec.lv; i++) markSpot(randSpot(), BLOCK);
	for (i=0; i<MAX_MONEY; i++) markSpot(randSpot(), MONEY);
	if (mode) {	// suvival mode bouns
		markSpot(randSpot(), FREEZE);
		markSpot(randSpot(), STUPID);
		markSpot(randSpot(), CUT);
		markSpot(randSpot(), KILL);
	}
}
int gameInit() {
	int choice,x,y;
	do {
		clrscr();
		printf("\n >>>>> %s <<<<<\t\t\tVersion: %s\t\tBy: %s\n", TITLE, VERSION, AUTHOR);
		printHR(cursor().Y);
		puts("\n >>>>> O B J E C T I V E");
		puts(" 1. Move the player 'Y' around to get as much money '$' as possible.");
		puts(" 2. Avoid being eaten by the SNAKE.");
		puts(" 3. The SNAKE will be more clever and will try to eat you! =]\n");
		puts(" >>>>> I N S T R U C T I O N");
		puts(" 1. You cannot move diagonally.");
		puts(" 2. The speed of the game will be automatically increased.");
		puts(" 3. Be sure to grab those MAGIC items.");
		printf(" 4. There is a total of %d levels. Good luck challenger! ^3^\n\n", MAX_LV);
		puts(" >>>>> C O N T R O L");
		puts(" Arrow Keys\tMove\n");
		printHR(cursor().Y);
		y=cursor().Y;
		puts("\n 1-9\tStart at the respective level.");
		puts("  0\tPlay Survival Mode. =P");
		puts("  H\tShow Highscore");
		puts("  C\tChange Name");
		puts(" ESC\tQuit the game.");
		printHR(cursor().Y);
		x = cursor().Y;
		char msg[50];
		sprintf(msg, " %s, what is your choice?", rec.name);
		do {
			flash(msg,0,y);
			choice=getkey();
			if (choice==ESC) return 0;	// exit
			else if (choice=='h') {
				printData();
				break;
			} else if (choice=='c') {
				changeName();
				break;
			}
		} while (choice<'0' || choice>'9');
	} while (choice<'0' || choice>'9');
	
	if (choice=='0') {
		mode = 1;
		rec.lv_start = MAX_LV;
	} else {
		mode = 0;
		rec.lv_start = choice-'0';
	}
	
	gotoxy(0, x);
	printf("You will now start with level %d\n", rec.lv_start);
	if (rec.lv_start==MAX_LV)
		puts("WoW You are so brave! Prepare to DIE! ... Urrr... Whatever.");
	do {flash("Press ENTER to start! @_@", 0, cursor().Y);}
	while (getkey()!=ENTER);

	for (x=0; x<WIDTH; x++) for (y=0; y<HEIGHT; y++) screen[x][y] = EMPTY;
	rec.score = eaten = killed = cmd = goldTaken = 0;
	moveTime = rec.elpsTime = 0;
	freezeTurns = -1;
	rec.lv = rec.slen = rec.lv_start;
	interval = 500/rec.lv;
	rec.clever = rec.lv*3;
	
	clrscr();
	printHR(HEIGHT);
	placeObjects();
	updateStatus(1,"");
	countDown(3);
	lastTime = clock();	// start the timer
	return 1;	// continue to play
}

/* game main */
void playerMove() {
	COORD newpos = player;
	int newcmd;
	if (kbhit()) {
		//do { newcmd = getkey(); } while ((newcmd==cmd) && kbhit());
		while (kbhit()) newcmd = getkey();
		switch (newcmd) {
			case ESC: eaten=-1; return;
			case UP: case DOWN: case LEFT: case RIGHT: cmd = newcmd;
		}
	}
	switch (cmd) {
		case UP: newpos.Y--; break;
		case DOWN: newpos.Y++; break;
		case LEFT: newpos.X--; break;
		case RIGHT: newpos.X++; break;
	}
	if (!validSpot(newpos)) return;
	switch (checkSpot(newpos)) {
		case EMPTY: case PLAYER: break;
		case SNAKE: case BLOCK: return;
		case SHEAD: if (freezeTurns>0) return; eaten=1; break;
		default: takeThing(checkSpot(newpos)); break;
	}
	removeSpot(player);
	if (!eaten) markSpot(player=newpos, PLAYER);
}
void snakeMove() {
	int i,x,y;
	if (freezeTurns>=0) {
		if (freezeTurns) {
			char msg[40];
			if (!killed)
				sprintf(msg, "The snake is FREEZED for %3d turns", freezeTurns);
			else
				sprintf(msg, "The snake will be revived in %3d turns", freezeTurns);
			updateStatus(3, msg);
			freezeTurns--;
			return;
		}
		freezeTurns = -1;
		if (killed) {	// revive
			killed = 0;
			markSpot(snake[0]=randSpot(), SHEAD);
			rec.slen = rec.lv;
			for (i=1; i<rec.slen; i++)
				markSpot(snake[i]=nearSpot(snake[i-1],0), SNAKE);
		}
		updateStatus(1, "");
		updateStatus(2, "The snake is BACK!");
		updateStatus(3, "");
	}
	COORD newpos = snake[0];
	if (random(1, 100) > rec.clever) newpos = nearSpot(snake[0],1);
	//if (1) newpos = nearSpot(snake[0],0);
	else {
		x = sign(player.X - snake[0].X);
		y = sign(player.Y - snake[0].Y);
		if (!x) newpos.Y += y;
		else if (!y) newpos.X += x;
		else if (random(0,1)) newpos.X += x;
		else newpos.Y += y;
		if (checkSpot(newpos)!=EMPTY && checkSpot(newpos)!=PLAYER)
			newpos = nearSpot(snake[0],1);
	}
	if (rec.slen==1 || checkSpot(snake[rec.slen-1])==SNAKE)
		removeSpot(snake[rec.slen-1]);
	for (i=rec.slen-1; i>0; i--) snake[i] = snake[i-1];
	snake[0] = newpos;
	if (checkSpot(snake[0])==PLAYER) eaten = 1;
	if (rec.slen>1) markSpot(snake[1], SNAKE);
	markSpot(snake[0], SHEAD);
}

/* main */
int main() {
	init();
	while (gameInit()) {
		do {
			updateTimer();
			if (currTime < moveTime) Sleep(1);
			else {	// next move
				moveTime = currTime+interval;
				playerMove();
				if (!eaten) snakeMove();
			}
		} while (!eaten);
		if (eaten==1) {
			updateStatus(2, "Game over. The snake has EATEN you! LOL");
			updateStatus(3, "");
			puts("");
			do {
				flash("Press ENTER to continue...", 0,cursor().Y);
			} while (getkey() != ENTER);
			if (insertData()) {	// check if entered top5
				updateStatus(2, "");
				updateStatus(3, "WoW! You entered >>>>> TOP 5 <<<<<");
				puts("");
				do {
					flash("Press ENTER to continue...", 0,cursor().Y);
				} while (getkey() != ENTER);
				printData();
			}
		}
	}
	clrscr();
	return 0;
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "conioext.h"
#define SIZE 100

enum { ROW, COL };
enum { UNKNOWN, BOX, SPACE, MAX_STATE };
enum { SUCCESS, EXTRABOX, NOSPACE, IMPOSS};
const char symbol[MAX_STATE] = { '.', '*', ' ' };

char sr[SIZE][SIZE], sc[SIZE][SIZE];		// solution
char cr[SIZE][SIZE], cc[SIZE][SIZE];		// constrain
char fcr[SIZE][SIZE], fcc[SIZE][SIZE];		// flipped constrain
char ur[SIZE], uc[SIZE];					// true if udpated
int row, col;								// storing board size
int depth=0, maxdepth=0;					// for deeper recusion
struct { int ei,ej;	/*entry point*/ } stack[SIZE];
char sdepth[SIZE][SIZE];					// for restore use
int showlevel, pgcount=0, fscount=0, start, end;
int debug=0;

void flip(char *arr, int size) {
	int i, tmp;
	for (i=0; i<size/2; i++) {
		tmp = arr[i];
		arr[i] = arr[size-i-1];
		arr[size-i-1] = tmp;
	}
}

void scaninput() {
	char c;
	row = col = 0;
	do {
		c = getchar();
		if (c=='(') scanf("%d", (int*)&cr[row][(int)++(*cr[row])]);
		else if (c==' ') scanf("%d", (int*)&cr[row][(int)++(*cr[row])]);
		else if (c==')') row++,c=getchar();
	} while (c!='\n');
	do {
		c = getchar();
		if (c=='(') scanf("%d", (int*)&cc[col][(int)++(*cc[col])]);
		else if (c==' ') scanf("%d", (int*)&cc[col][(int)++(*cc[col])]);
		else if (c==')') col++, c=getchar();
	} while (c!='\n' && c!=EOF);
	
	int i;
	for (i=0; i<row; i++) {
		memcpy(fcr[i],cr[i],sizeof(cr[i]));
		flip(fcr[i]+1,*fcr[i]);
		ur[i] = 1;
	}
	for (i=0; i<col; i++) {
		memcpy(fcc[i],cc[i],sizeof(cc[i]));
		flip(fcc[i]+1,*fcc[i]);
		uc[i] = 1;
	}
	memset(sr,UNKNOWN,sizeof(sr));
	memset(sc,UNKNOWN,sizeof(sc));
}

int finish() {
	int i,j;
	for (i=0; i<row; i++) for (j=0; j<col; j++)
		if (sr[i][j] == UNKNOWN) return 0;
	return 1;
}

void prtboard(int clear) {
	if (!showlevel) return;
	if (clear) clrscr(); else gotoxy(0,0);
	int r,c;
	for (r=0; r<row; r++,puts("")) {
		for (c=0; c<col; c++)
			printf(" %c", symbol[(int)sr[r][c]]);
	}
}

void prtsolrc(int type, int i) {
	int r,c;
	if (type==ROW) {
		gotoxy(0, i);
		for (c=0; c<col; c++) printf(" %c", symbol[(int)sr[i][c]]);
	} else {
		for (r=0; r<row; r++) {
			gotoxy(i*2+1,r);
			printf("%c", symbol[(int)sr[r][i]]);
		}
	}
	gotoxy(0, row+1);
}

void prtsol(char sol[SIZE], int len) {
	int i;
	for (i=0; i<len; i++)
		printf("%c ",symbol[(int)sol[i]]);
	puts("");
}

int pattgen(char *cons, char *sol, char *tmp, int d, int ci, int bc, int w) {
	// d=depth	ci=constrain_index	bc=box_count
	if (debug) {
		if (!d) {
			int i;
			//printf("con: ");
			for (i=1; i<=*cons; i++) printf("%d ", cons[i]);
			printf("\n");
			prtsol(sol, w);
		} else prtsol(tmp, d), getch();
	}
	if (d==w) {
		if (ci<*cons) return IMPOSS;						// still have segment to fill
		if (ci==*cons && bc<cons[ci]) return IMPOSS;		// last segment not finished
		return SUCCESS;
	}
 	switch (sol[d]) {
		case BOX: {
			int end = d+cons[ci]-bc;
			if (end>w) return IMPOSS;						// not enough space even at end
			while (d<end) {
				if (sol[d]==SPACE) return NOSPACE;			// not enough space
				tmp[d++] = BOX; bc++;
			}
			if (end<w&&sol[end]==BOX) return EXTRABOX;		// extra box after segment
			break;
		}
		case SPACE: {
			if (bc && bc<cons[ci]) return NOSPACE;			// not enough space
			if (ci<=*cons && bc==cons[ci]) { bc=0; ci++; }	// statified current segment
			if (ci>*cons) while (d<w) {						// last segment reached
				if (sol[d]!=BOX) tmp[d++] = SPACE;
				else return EXTRABOX;						// extra segment
			} else do tmp[d++] = SPACE;						// fill continuous space
				while (d<w && sol[d]==SPACE);
			break;
		}
		case UNKNOWN: {
			if (ci>*cons || bc==cons[ci]) {					// must be space
				if (ci<=*cons) { bc=0; ci++; }				// statified current segment
				if (ci>*cons) while (d<w) {					// last segment reached
					if (sol[d]!=BOX) tmp[d++] = SPACE;
					else return EXTRABOX;					// extra segment
				} else do tmp[d++] = SPACE;					// fill continuous space
					while (d<w && sol[d]==SPACE);
			} else {										// unknown case
				// try a box first
				int ret=SUCCESS, begin=d, end=d+cons[ci], i;
				if (end>w) return IMPOSS;
				while (d<end && ret==SUCCESS) if (sol[d]==SPACE) {
					ret = NOSPACE;							// not enough space
					// box will lead to no space, determine if fill the gap or not
					for (i=begin; i<w; i++) if (sol[i]!=UNKNOWN) break;
					if (i<w && sol[i]==SPACE) {				// current segment no space
						for (d=begin; sol[d]!=SPACE; d++) tmp[d] = SPACE;
						while (sol[d]==SPACE) tmp[d++]=SPACE;
						break;
					} else ret = EXTRABOX;					// next segment no space
				} else tmp[d++] = BOX;
				if (ret == NOSPACE) break;
				
				if (end<w&&sol[end]==BOX) ret = EXTRABOX;	// extra box after segment
				if (ret == SUCCESS) {						// successfully placed the current segment
					ret = pattgen(cons,sol,tmp,d,ci,cons[ci],w);
					if (ret==SUCCESS || ret==IMPOSS) return ret;
				}
				// current cell cannot be a box, reset and try to join current segment to next segment
				d=begin, begin=0, end=0;
 				for (i=d; i<w; i++) if (sol[i]==BOX) {		// find if a segment exist
					if (!begin) begin=end=i; else end++;
				} else if (end) break;
				if (begin && end) {							// a segment exist
					// segment length too long
					if (end-begin+1>cons[ci]) return EXTRABOX;		
					end -= cons[ci]-1;						// calculate spaces to be filled
					if (end>d) {							// have spaces to fill
						while (d<end) tmp[d++] = SPACE;
						if (pattgen(cons,sol,tmp,d,ci,0,w) == SUCCESS)
							return SUCCESS;					// return if SUCCESS
					}
				}
				// try a single space if still have conflict after the above whole work
				if (sol[d]==BOX) return EXTRABOX;			// current cell must be box
				tmp[d++] = SPACE;							// otherwise just fill a space
			}
			break;
		}
	}
	return pattgen(cons,sol,tmp,d,ci,bc,w);
}

int combine(char *cons, char *left, char *right, int type, int i) {
	char begin[SIZE], end[SIZE]={-1};		// constrains begin end range
	int d,ci,prev=UNKNOWN,updated=0,width=type?row:col;
	char *sol, (*tosol)[SIZE], *u;			// pointer to array
	if (type==ROW) { sol=sr[i]; tosol=sc; u=uc; width=col; }
	else { sol = sc[i]; tosol = sr; u = ur; width=row; }

	// constrain is 0
	if (!cons[1]) {				
		for (d=0; d<width; d++) {
			if (sol[d]!=SPACE) {
				sol[d] = tosol[d][i] = SPACE;
				sdepth[type?d:i][type?i:d] = depth;
				updated = u[d] = 1;
			}
		}
		return updated;
	}
	
	// find begin end range
	for (prev=UNKNOWN,d=0,ci=1; d<width; d++) {
		if (left[d]==SPACE && prev==BOX) ci++;
		else if (left[d]==BOX && prev!=BOX) begin[ci] = d;
		prev = left[d];
	}
	for (prev=UNKNOWN,d=width-1,ci=*cons; d>=0; d--) {
		if (right[d]==SPACE && prev==BOX) ci--;
		else if (right[d]==BOX && prev!=BOX) end[ci] = d;
		prev = right[d];
	}
	
	for (d=0,ci=1; ci<=*cons; ci++) {
		// fill uncoverd cell (to start) with space
		for (d=end[ci-1]+1;d<begin[ci];d++)
			if (sol[d] != SPACE) {
				sol[d] = tosol[d][i] = SPACE;
				sdepth[type?d:i][type?i:d] = depth;
				updated = u[d] = 1;
			}
			
		// fill overlapped cell with box
		for (d=end[ci]+1-cons[ci]; d<cons[ci]+begin[ci]; d++)
			if (sol[d] != BOX) {
				sol[d] = tosol[d][i] = BOX;
				sdepth[type?d:i][type?i:d] = depth;
				updated = u[d] = 1;
			}
		
		// segment length satisfies neighbour constrains
		if (ci>1 && end[ci-1]-begin[ci]+1==cons[ci] && cons[ci]==cons[ci-1]) {
			int mustbespace=1;
			for (d=begin[ci]; d<=end[ci-1]; d++)
				if (sol[d]!=BOX) mustbespace=0;
			if (mustbespace) {
				if (sol[d=begin[ci]-1] != SPACE) {
					sol[d] = tosol[d][i] = SPACE;
					sdepth[type?d:i][type?i:d] = depth;
					updated = u[d] = 1;
				}
				if (sol[d=end[ci-1]+1] != SPACE) {
					sol[d] = tosol[d][i] = SPACE;
					sdepth[type?d:i][type?i:d] = depth;
					updated = u[d] = 1;
				}
			}
		}
	}
	// fill uncoverd cell (to last) with space
	for (d=end[(int)*cons]+1; d<width; d++)
		if (sol[d] != SPACE) {
			sol[d] = tosol[d][i] = SPACE;
			sdepth[type?d:i][type?i:d] = depth;
			updated = u[d] = 1;
		}
	return updated;
}

int solve(char type, int i) {
	char left[SIZE], right[SIZE], fsol[SIZE];
	int width = type?row:col;
	char *cons, *fcons, *sol;	// pointer to array
	if (type == ROW) { cons=cr[i]; fcons=fcr[i]; sol=sr[i]; }
	else { cons=cc[i]; fcons=fcc[i]; sol=sc[i]; }
	
	pgcount++;
	//if (fscount==3 && type==COL && i==21) debug=1;
	if (pattgen(cons,sol,left,0,1,0,width)!=SUCCESS) return IMPOSS;	// imposs
	memcpy(fsol,sol,sizeof(fsol));
	flip(fsol,width);
	pgcount++;
	pattgen(fcons,fsol,right,0,1,0,width);
	flip(right,width);
	return combine(cons, left, right, type, i);
}

int findentry() {
	if (depth==SIZE) return IMPOSS;	// reached max stack size
	int i=0,j=0;
	while (i<row && j<col) {
		if (sr[i][j]==UNKNOWN) break;
		if (++j==col) j=0,i++;
	}
	stack[depth].ei = i;
	stack[depth].ej = j;
	sc[j][i] = sr[i][j] = BOX;	// set it to be a BOX
	ur[i] = uc[j] = 1;
	sdepth[i][j] = depth++;
	if (depth>maxdepth) maxdepth=depth;
	if (showlevel==4) {
		puts("not able to solve, find an entry point");
		printf("ei=%d\tej=%d\n",i,j);
		while (getch()!='\n');
		prtboard(1);
	}
	return SUCCESS;	// success
}

int restore() {
	if (!depth) return IMPOSS;	// imposs

	// remove all decisions made after entry point
	int i,j;
	for (i=0; i<row; i++) for (j=0; j<col; j++)
		if (sdepth[i][j]==depth) {
			sdepth[i][j] = 0;
			sr[i][j] = sc[j][i] = UNKNOWN;
		}
	
	// test another option at entry point
	i=stack[depth-1].ei; j=stack[depth-1].ej;
	if (sr[i][j]==BOX) {
		sc[j][i] = sr[i][j] = SPACE;
		if (showlevel==4) {
			puts("contradition, try another option at entry point");
			printf("ei=%d\tej=%d\n",i,j);
			while (getch()!='\n');
			prtboard(1); getch();
		}
		ur[i] = uc[j] = 1;
		return SUCCESS;	// restored with another choice
	}
	if (showlevel==4) puts("contradition, restore 1 depth");
	depth--;
	return restore();	// restore one more level
}

int main(int argc, char **argv) {
	freopen("in.txt", "r", stdin);
	//freopen("out.txt", "w", stdout);
	showlevel=1;
	if (argv[1]) {
		if (!strcmp(argv[1],"0")) showlevel=0;
		else if (!strcmp(argv[1],"2")) showlevel=2;
		else if (!strcmp(argv[1],"3")) showlevel=3;
		else if (!strcmp(argv[1],"4")) showlevel=4;
	}
	
	scaninput();
	if (col*2+1>80 || row+6>25)
		conResize(row+6>25?row+6:25,col*2+1>80?col*2+1:80);
	if (showlevel>=2) prtboard(1);
	int updated,i,ret;
	start=clock();
	while (1) {
		updated = 1;
		while (updated) {
			fscount++;
			updated = 0;
			for (i=0; i<row; i++) {
				if (ur[i] && (ret=solve(ROW, i))) {
					if (ret==IMPOSS) {
						if (showlevel>=3) {
							printf("row %d\n", i);
							getch();
						}
						break;	// imposs
					}
					updated = 1;
					if (showlevel>=2) {
						prtsolrc(ROW,i);
						printf("updated:\trow[%2d]\n",i);
						if (showlevel>=3) if (getch()=='+') showlevel=2;
					}
				}
				ur[i] = 0;
			}
			if (ret==IMPOSS) break;
			for (i=0; i<col; i++) {
				if (uc[i] && (ret=solve(COL, i))) {
					if (ret==IMPOSS) {
						if (showlevel>=3) {
							printf("col %d\n", i);
							getch();
						}
						break;	// imposs
					}
					updated = 1;
					if (showlevel>=2) {
						prtsolrc(COL,i);
						printf("updated:\tcol[%2d]\n",i);
						if (showlevel>=3) if (getch()=='+') showlevel=2;
					}
				}
				uc[i] = 0;
			}
			if (ret==IMPOSS) break;
			if (showlevel>=2 && updated) {
				//prtboard(0);
				printf("after %3d full scan\n",fscount);
				if (showlevel==2 && getch()=='-') showlevel=3;
			}
		}
 		if (!finish() || ret==IMPOSS) {
			if (ret==IMPOSS) {	// imposs
				if (restore()==IMPOSS) {
					end = clock();
					prtboard(1);
					puts("impossible");
					break;
				}
			} else if (findentry()==IMPOSS) {	// still possible
				end = clock();
				prtboard(1);
				puts("not able to solve, need larger stack size");
				break;
			}
		} else {
			end = clock();
			prtboard(1);
			puts("solved");
			break;
		}
	}
	if (showlevel<=1) printf("time=%3dms\t", (int)(end-start));
	printf("patterns gened=%4d\tfull scans=%3d\tmax depth=%2d\n",pgcount,fscount,maxdepth);
	return 0;
}
program game_snake;
uses wincrt, winprocs, windos;
(************************************
*            declaration            *
************************************)
type
	coordinate = record			{ coordinate type }
		x, y: integer;
	end;
	thing = (				{ list of thing }
		empty_t, player_t, snakehead_t, snake_t, money_t,
		freeze_t, stupid_t, cutsnake_t, lowdiff_t, lasthelp_t,
		block_t, border_t);
	hsrec = record				{ highscore record type }
		name: string[20];
		mode, score, sta_lv, end_lv, clevn, slen: integer;
		time_p: real;
	end;
const
	{ game broad config }
	version		= 'v1.53';
	author		= 'Tom Lau';
	data_path	= 'score';		{ highscore path }
	hsrec_max	= 5;			{ maximum records in each mode }
	mode_max	= 2;			{ maximum modes }
	height		= 20;
	width		= 33;
	char_t: array[thing] of char = (	{ char for thing }
		' ', chr(159), chr(167), '+', '$',
		'F', 'S', 'C', 'D', ' ',
		chr(127), '|');

	{ command char }
	upcmd		= 'W';
	downcmd		= 'S';
	leftcmd		= 'A';
	rightcmd	= 'D';
	refscrcmd	= 'R';
	esccmd		= chr(27);
	hscmd		= 'H';			{ highscore command }
	chnamecmd	= 'C';			{ change name command }

	{ object config }
	money_score	= 20;			{ base point of a gold }
	help_score	= 100;			{ score of eating a help item }
	kill_score	= 250;			{ score for killing a snake }
	freezeturns	= 100;			{ base turns to freeze the snake }
	reviveturns	= 250;			{ base turns for the snake to revive }
	clever_max	= 90;			{ maximum cleverness the snake can reach }

	poss_help_item	= 5;			{ base possibility of giving a help item in % }
	poss_t: array[freeze_t..pred(lasthelp_t)] of integer = (
		37, 10, 47, 6);			{ must add up to 100% }
	poss_total	= 100;

	lv_up_nog	= 5;			{ number of gold taken to level up }
	lv_max		= 25;			{ maximum level }
	lv_time		= 0.6;			{ base time for lv one in s }
	snakelen_max	= (lv_max+1) div 2;	{ maximum length of the snake }

	{ string const }
	hr = '-------------------------------------------------------------------';
	cont = 'Press any key to continue...';
	enter = 'Press enter to continue...';

var
	player: coordinate;
	snake: array[1..snakelen_max] of coordinate;
	help_t: array[money_t..pred(lasthelp_t)] of coordinate;
	block: array[1..lv_max] of coordinate;
	diff, gold_taken: integer;

	screen: array[0..width, 0..height] of thing;
	poss: array[1..poss_total] of thing;	{ for generating the possibility sequence }
	freeze_c: integer;			{ for freezing function }
	killed, eaten: boolean;			{ snake is killed or player is eaten }

	hs_data: text;
	user_m: array[1..mode_max] of integer;	{ records in each mode }
	hs_rec: array[1..mode_max, 1..hsrec_max] of hsrec;	{ storing highscore records }
	cur_rec: hsrec;				{ storing current user record}

	cmd: char;				{ storing player's command }
	sta_time, time_pass: real;

(******************************************
*            general functions            *
******************************************)
function rand(min, max: integer): integer;
{ returns a random integer between min and max }
begin
	rand:= min + random(max-min+1);
end;	{ rand }

procedure delay(time: real; can_break: boolean);
{ for delaying time in s }
var tfinal, tleft: real;
begin
	tfinal:= gettickcount/1000 + time;	{ get current time }
	tleft:= tfinal -gettickcount/1000;
	if can_break then while (not keypressed) and (tleft > 0) do
		tleft:= tfinal -gettickcount/1000
	else while tleft > 0 do
		tleft:= tfinal -gettickcount/1000;
end;	{ delay }

function flash(msg: string; x, y: integer): char;
{ a fancy function for flashing a msg and return an input char }
const
	next_time = 0.02;
	pattern = 5;
var
	len, count: integer;
	i, j: integer;
begin
	count:= 0;
	len:= length(msg);
	repeat
		inc(count);
		case count mod pattern of
			1: begin			{ pattern - show the msg }
				cursorto(x, y);
				for i:= 1 to len do begin
					clreol;
					write(msg[i]);
					delay(next_time, true);
				end;
			end;
			0, 2..pattern-1: begin		{ other pattern - flash each char }
				for i:= 1 to len+1 do begin
					cursorto(x, y);
					for j:= 1 to len do
						if i = j then write(' ') else write(msg[j]);
					delay(next_time, true);
				end;
				if count mod pattern = 0 then count:= 0;
			end;
		end;
		if not keypressed then delay(next_time*50, true);
	until keypressed;
	flash:= readkey;
end;	{ flash }

function ckspot(coord: coordinate; forwhat: thing): boolean;
{ returns true if the position is valid and is the thing to find }
begin
	with coord do if (x in [0..width]) and (y in [0..height]) then
		ckspot:= screen[x, y] = forwhat
	else ckspot:= false;
end;	{ ckspot }

procedure mark(coord: coordinate; forwhat: thing);
{ mark the location as the given thing }
begin
	with coord do begin
		cursorto(x ,y);
		write(char_t[forwhat]);
		screen[x, y]:= forwhat;
	end;
end;	{ mark }

procedure remove(coord: coordinate);
{ removes whatever is at the coordinates from the screen }
begin
	mark(coord, empty_t);
end;	{ remove }

procedure randspace(var newpos: coordinate; forwhat: thing);
{ generate a position on the screen that has not been used }
begin
	with newpos do begin
		repeat
			x:= rand(0, width);
			y:= rand(0, height);
		until ckspot(newpos, empty_t);
		mark(newpos, forwhat);
	end;
end;	{ randspace }

procedure gettimepass;			{ return the time passed }
begin
	time_pass:= gettickcount/1000 - sta_time;
end;	{ gettimepass }

(***************************************
*            refresh screen            *
***************************************)
procedure write_border;
var y: integer;
begin
	for y:= 0 to height+1 do begin
		cursorto(width+1, y);
		write(char_t[border_t]);
	end;
end;	{ write_border }

procedure refhud(line: integer; msg: string);
const
	s: array[1..4] of string[16] = (
		'Level:', '  Cleverness:',
		'%  Snake length:', '  Time passed:');
begin
	cursorto(0, height+line);
	clreol;
	with cur_rec do case line of
		1: writeln(s[1], diff, '  $', score, s[2], clevn, s[3], slen, s[4], time_pass:0:1, 's');
		2: if freeze_c > 0 then
			if not killed then
				writeln('The snake is freezed for ', freeze_c, ' turns.')
			else writeln('It will be revived in ', freeze_c, ' turns.');
		3: write(msg);
	end;
end;	{ refhud }

procedure refscr;
{ refresh screen }
var x, y: integer;
begin
	clrscr;
	for y:= 0 to height do for x:= 0 to width do begin
		cursorto(x, y);
		write(char_t[screen[x,y]]);
	end;
	write_border;
	refhud(1, '');
	refhud(3, '');
end;	{ refscr }

(*****************************************
*            things functions            *
*****************************************)
procedure rand_poss(forwhat: thing; poss_count: integer);
var
	i, rnd: integer;
	found: boolean;
begin
	randomize;
	for i:= 1 to poss_count do
	repeat
		found:= false;
		rnd:= rand(1, poss_total);
		if poss[rnd] = empty_t then begin
			poss[rnd]:= forwhat;
			found:= true;
		end;
	until found;
end;	{ rand_poss }

procedure rand_thing_init;				{ generate a random sequence }
var
	i: integer;
	forwhat: thing;
begin
	for forwhat:= freeze_t to pred(pred(lasthelp_t)) do
		rand_poss(forwhat, poss_t[forwhat]);
	for i:= 1 to poss_total do if poss[i] = empty_t then
		poss[i]:= pred(lasthelp_t);
end;	{ rand_thing_init }

procedure rand_thing;					{ random a help item }
var
	forwhat: thing;
	rnd: integer;
begin
	if rand(1, 100) <= (poss_help_item + round(diff/1.5)) then begin
		rnd:= rand(1, poss_total);
		forwhat:= poss[rnd];
		if help_t[forwhat].x = -1 then randspace(help_t[forwhat], forwhat);
	end;
end;	{ rand_thing }

procedure takething(forwhat: thing);			{ handle taking an item }
const
	msg: array[1..6] of string = (
		'The snake is lengthened!', 'The snake is freezed! +$',
		'The snake is killed! +$', 'The snake is more stupid! +$',
		'The snake is shortened! +$', 'Level lowered! +$');
var s: string[5];
begin
	if forwhat <> money_t then begin
		cur_rec.score:= cur_rec.score + help_score;
		str(help_score, s);

	end;
	with cur_rec do case forwhat of
		money_t: begin
			score:= score + money_score + (diff-1)*5;
			randspace(help_t[money_t], money_t);
			rand_thing;

			inc(gold_taken);
			if (diff<lv_max) and ((gold_taken mod lv_up_nog) = 0) then begin
				inc(diff);
				randspace(block[diff], block_t);
			end;
			if (clevn < clever_max) and (gold_taken mod 2 =0) then inc(clevn);
			if (not killed) and (slen<snakelen_max) and (gold_taken mod (lv_up_nog*2) =0) then begin
				inc(slen);
				refhud(3, msg[1]);
			end;
		end;
		freeze_t: if not killed then begin
			freeze_c:= freezeturns;
			refhud(3, msg[2] +s);
		end
		else  refhud(3,msg[3] +s);
		stupid_t: begin
			if (clevn -10) < 0 then clevn:= 0
			else dec(clevn, 10);
			refhud(3, msg[4] +s);
		end;
		cutsnake_t: if not killed then begin
			dec(slen);
			if slen = 0 then begin
				remove(snake[1]);
				freeze_c:= reviveturns;
				killed:= true;
				refscr;
				str(kill_score, s);
				refhud(3, msg[3] +s);
			end
			else begin
				remove(snake[slen +1]);
				refhud(3, msg[5] +s);
			end;
		end
		else refhud(3, msg[3] +s);
		lowdiff_t: if diff>1 then begin
        		remove(block[diff]);
			dec(diff);
			refhud(3, msg[6] +s);
		end;
	end;
	refhud(1, '');
	if forwhat <> money_t then help_t[forwhat].x:= -1;	{ remove the item }
end;	{ takething }

procedure placenearby(var near, ori: coordinate; forwhat: thing);
{ placenearby finds a free coordinate adjacent to the argument coordinate and places the thing there }
var pass, search_a: integer;
begin
	pass:= 0;
	search_a:= 0;
	with near do repeat
		inc(pass);
		near:= ori;
		case rand(0, 3) of
			0: dec(x, 1 +search_a);
			1: inc(x, 1 +search_a);
			2: dec(y, 1 +search_a);
			3: inc(y, 1 +search_a);
		end;
		if pass >= 100 then begin		{ must be cannot find a space }
			inc(search_a);			{ increase the search area }
			pass:= 0;
		end;
	until ckspot(near, empty_t) or ckspot(near, player_t);
	mark(near, forwhat);
end;	{ placenearby }

procedure placeobjects;
{ position all of the items in the game making sure that none of them overlap }
var
	snakebody, i: integer;
	forwhat: thing;
begin
	for forwhat:= succ(money_t) to pred(lasthelp_t) do
		help_t[forwhat].x:= -1;
	randspace(snake[1], snakehead_t);
	for snakebody:= 2 to cur_rec.slen do
		placenearby(snake[snakebody], snake[snakebody - 1], snake_t);
	randspace(player, player_t);
	randspace(help_t[money_t], money_t);
	for i:= 1 to diff do randspace(block[i], block_t);
end;	{ placeobjects }

(************************************
*            player move            *
************************************)
procedure playermove;
{ read the player's move from the keyboard }
var
	oldpos: coordinate;			{ storing the old position of player }
	newcmd: char;				{ read the new command }
begin
	if keypressed then begin
		repeat
			newcmd:= upcase(readkey);
		until (cmd <> newcmd) or (not keypressed);
		case newcmd of
			refscrcmd: refscr;
			upcmd, downcmd, leftcmd, rightcmd: cmd:= newcmd
		end;
	end;
	with player do begin
		oldpos:= player;
		case cmd of
			upcmd: if y > 0 then dec(y); 		{ else y:= height; }
			downcmd: if y < height then inc(y);	{ else y:= 0; }
			leftcmd: if x > 0 then dec(x);	 	{ else x:= width; }
			rightcmd: if x < width then inc(x);	{ else x:= 0; }
		end;

		case screen[x, y] of
			snakehead_t: if freeze_c = 0 then eaten:= true else player:= oldpos;
			money_t..pred(lasthelp_t): takething(screen[x, y]);
			block_t, snake_t: player:= oldpos;
		end;
	end;
	remove(oldpos);
	if not eaten then mark(player, player_t);
end;	{ playermove }

(***********************************
*            snake move            *
***********************************)
procedure snakemove;
{ snake moves randomly at first, then it goes more directly toward the player }
	function sign(x: integer): integer;
	{ return which direction is toward the player }
	begin
		if x = 0 then sign:= 0
		else if x > 0 then sign:= 1
		else sign:= -1;
	end;	{ sign }
var
	newpos: coordinate;
	body, signx, signy: integer;
begin
	if killed then begin
		killed:= false;
		cur_rec.slen:= (diff+1) div 2;
		randspace(snake[1], snake_t);
		for body:= 2 to cur_rec.slen do
			placenearby(snake[body], snake[body -1], snake_t);
		refhud(3, 'The snake is revived!');
	end;

	if rand(1, 100) > cur_rec.clevn then placenearby(newpos, snake[1], snakehead_t)
	else with newpos do begin
		newpos:= snake[1];
		signx:= sign(player.x - x);
		signy:= sign(player.y - y);
		case signx of
			1, -1: if signy = 0 then x:= x + signx
				else case rand(0,1) of
					0: x:= x + signx;
					1: y:= y + signy;
				end;
			0: y:= y + signy;
		end;
		if ckspot(newpos, player_t) or ckspot(newpos, empty_t) then mark(newpos, snakehead_t)
		else placenearby(newpos, snake[1], snakehead_t);
	end;

	if (newpos.x = player.x) and (newpos.y = player.y) then eaten:= true;
	mark(snake[1], snake_t);
	remove(snake[cur_rec.slen]);
	for body:= cur_rec.slen downto 2 do snake[body]:= snake[body -1];
	snake[1]:= newpos;
end;	{ snakemove }

(***************************************
*            initialization            *
***************************************)
procedure welcome_scr;
begin
	writeln;
	writeln(hr); writeln;
	writeln('    SSSS   N   N    AAA    K   K   EEEEE');
	writeln('   S       NN  N   A   A   K  K    E    ');
	writeln('    SSS    N N N   A   A   KKK     EEEEE');
	writeln('       S   N  NN   AAAAA   K K     E    ');
	writeln('   SSSS    N   N   A   A   K   K   EEEEE');
	writeln; writeln(hr);
	repeat until chr(13) = flash(enter, 6, 14);
end;	{ welcome_scr }

procedure input_name;					{ get user's name }
var input: string;
begin
	clrscr;
	writeln('^_^ Please input your name for recording score.');
	writeln('Only the first 20 letters will be counted.');
	writeln('If you leave it blank, the following name will be displayed.');
	writeln(hr);
	with cur_rec do begin
		if name = '' then name:= 'Player';
		writeln('Your current name: ', name);
		writeln(hr); writeln; readln(input);
		if input <> '' then name:= input;
	end;
end;	{ input_name }

procedure init;
begin
	assign(hs_data, data_path);
	randomize;
	rand_thing_init;
	welcome_scr;
	input_name;
end;	{ init }

(***********************************
*            high score            *
***********************************)
function file_exist(filename: string): boolean;
{ copied from pascal's sample code which will return true if the file exists }
var F: text;
begin
	{$I-} 					{ ignore any I/O result that is the error when file not found }
	assign(F, filename);
	reset(F);
	close(F);
	{$I+}
       	file_exist:= IOResult = 0;
end;	{ file_exist }

procedure clear_rec(temp: hsrec);		{ clear a highscore record }
begin
	with temp do begin
		name:= ''; mode:= 0; score:= 0; sta_lv:= 0;
		end_lv:= 0; clevn:= 0; slen:= 0; time_p:= 0.0;
	end;
end;	{ clear_data }

procedure get_data;
const item_max = 8;
var
	i, j, user, item, code: integer;
	invalid: boolean;			{ true if the file is invalid }
	s: array[1..item_max] of string[20];	{ storing the items as string }
	temp: char;
	temp_rec: hsrec;
begin
	for i:= 1 to mode_max do begin
		for user:= 1 to user_m[i] do clear_rec(hs_rec[i, user]);
		user_m[i]:= 0;
	end;

	if file_exist(data_path) then begin
		user:= 0;
		reset(hs_data);
		while (not eof(hs_data)) and (user <= hsrec_max*mode_max) do begin
			for i:= 1 to item_max do s[i]:= '';
			item:= 1;
			while not eoln(hs_data) do begin
				read(hs_data, temp);
				if temp = ',' then inc(item)
				else if item <= item_max then s[item]:= s[item] + temp;
			end;
			if item = item_max then with temp_rec do begin
				name:= s[1];
				val(s[2], mode, code); val(s[3], score, code);
				val(s[4], sta_lv, code); val(s[5], end_lv, code);
				val(s[6], clevn, code); val(s[7], slen, code);
				val(s[8], time_p, code);
				if mode in [1..mode_max] then if (user_m[mode] < hsrec_max) then begin
					inc(user);
					inc(user_m[mode]);
					hs_rec[mode, user_m[mode]]:= temp_rec;
				end;
			end;
			readln(hs_data);
		end;
		close(hs_data);
	end;
end;	{ get_data }

procedure show_data;
const
	msg1	= 'No. Name                 Score  Lv     Snake       Snake   Time';
	msg2	= '                                       cleverness  length  passed';
	sp = '  ';
var i, m, len: integer;
begin
	for m:= 1 to mode_max do begin
		case m of
			1: writeln('Standard mode', '>>>>>>>>>> Sort by Score <<<<<<<<<<':50);
			2: writeln('Survival mode', '>>>>>>>>>> Sort by Time passed <<<<<<<<<<':53);
		end;
		writeln(msg1); writeln(msg2); writeln(hr);
		for i:= 1 to hsrec_max do if i > user_m[m] then writeln(i:2)
		else with hs_rec[m, i] do begin
			len:= length(name);
			writeln(i:2, sp, name, sp:21-len, score:5, sp, sta_lv:2, '-', end_lv:2, sp, clevn:9, '%', sp, slen:6, sp, time_p:5:1, 's');
		end;
		writeln(hr);
	end;
end;	{ show_data }

function comp_rec(a, b: hsrec; m: integer): boolean;
{ return true if a is smaller and false if a is larger }
begin
	with a do if m = 0 then
		comp_rec:= (score < b.score) or ((score = b.score) and (name < b.name)) or
			((score = b.score) and (name < b.name) and (end_lv < b.end_lv))
	else comp_rec:= (time_p < b.time_p) or ((time_p = b.time_p) and (score < b.score)) or
			((time_p = b.time_p) and (score = b.score) and (name < b.name));
end;	{ comp_rec }

procedure sort_data;
var
	i, j, m: integer;
	found: boolean;
	temp: hsrec;
begin
	for m:= 1 to mode_max do begin
		i:= 1; found:= true;
		while (i<= user_m[m]-1) and found do begin
			found:= false;
			for j:= 1 to user_m[m] -i do begin
				found:= comp_rec(hs_rec[m, j], hs_rec[m, j+1], m);
				if found then begin
					temp:= hs_rec[m, j];
					hs_rec[m, j]:= hs_rec[m, j+1];
					hs_rec[m, j+1]:= temp;
				end;
			end;
			inc(i);
		end;	{ bubble_sort }
	end;
end;	{ sort_data }

procedure save_data;
var i, m: integer;
begin
	rewrite(hs_data);
	for m:= 1 to mode_max do for i:= 1 to hsrec_max do if i <= user_m[m] then with hs_rec[m, i] do
		writeln(hs_data, name, ',', mode, ',', score, ',', sta_lv, ',', end_lv, ',', clevn, ',', slen, ',', time_p:0:1);
	close(hs_data);
end;	{ save_data }

function insert_data(temp: hsrec): boolean;
var m: integer;
begin
	insert_data:= false;
	m:= temp.mode;
	get_data;
	if (user_m[m] < hsrec_max) or comp_rec(hs_rec[m, user_m[m]], temp, m) then begin
		insert_data:= true;
		if (user_m[m] < hsrec_max) then inc(user_m[m]);
		hs_rec[m, user_m[m]]:= temp;
		sort_data;
		save_data;
	end;
end;	{ insert_data }

function lead_0(w: word): string;		{ add leading zero }
var s: string;
begin
	str(w:0, s);
	if length(s) = 1 then s:= '0' + s;
	lead_0:= s;
end;	{ lead_0 }

procedure highscore;
var
	y, m, d, h, min, sec, null: word;
	date_s, time_s: string[10];
begin
	get_data;
	clrscr;
	getdate(y, m, d, null);
	gettime(h, min, sec, null);
	date_s:= lead_0(d) + '/' + lead_0(m) + '/' + lead_0(y);
	time_s:= lead_0(h) + ':' + lead_0(min) + ':' + lead_0(sec);
	writeln('High score                      Last updated: ', date_s, '  ', time_s);
	writeln(hr);
	show_data;
	repeat until chr(13) = flash(enter, 0, cursor.y);
end;	{ highscore }

(**********************************
*            game main            *
**********************************)
procedure game_inst(var ans: char);
var
	code: integer;
	valid: boolean;
begin
	clrscr;
	writeln('   S N A K E', version: 40, '  By ', author);
	writeln(hr);
	writeln('          >>>>>>>>>> O B J E C T I V E <<<<<<<<<<');
	writeln(' 1. Control the player ''Y'' to get as much money ''$'' as possible.');
	writeln(' 2. Advoid being eaten by the SNAKE.');
	writeln(' 3. The SNAKE will be more clever and will try to eat you! =]');
	writeln('        >>>>>>>>>> I N S T R U C T I O N <<<<<<<<<<');
	writeln(' 1. You cannot move diagonally.');
	writeln(' 2. The speed of the game will be automatically increased.');
	writeln(' 3. There is a total of ', lv_max, ' lv. Good luck. ^3^');
	writeln('            >>>>>>>>>> C O N T R O L <<<<<<<<<<');
	writeln(' ', upcmd, '  = Up');
	writeln(' ', downcmd, '  = Down');
	writeln(' ', leftcmd, '  = Left');
	writeln(' ', rightcmd, '  = Right');
	writeln(' ', refscrcmd, '  = Refresh the screen if it is messed up');
	writeln(hr);
	writeln('1-9 = Start at the respective lv.');
	writeln(' 0  = Play Survival mode. =P');
	writeln(' ', hscmd, '  = View highscore.');
	writeln(' ', chnamecmd, '  = Change your name.');
	writeln('Esc = Quit the game.');
	writeln;
	repeat
		valid:= true;
		ans:= upcase(flash('Your choice ?', cursor.x, cursor.y));
		case ans of
			'1'..'9': val(ans, diff, code);
			'0': diff:= lv_max;
			hscmd, esccmd, chnamecmd: begin end;
			else begin
				valid:= false;
				cursorto(0, cursor.y-1);
				writeln('No such choice');
			end;
		end;
	until valid;
end;	{ game_inst }

procedure game_init;				{ prepare for a new game }
var
	x, y: integer;
	s: char;
begin
	clrscr;
	for x:= 0 to width do for y:= 0 to height do
		screen[x,y]:= empty_t;
	killed:= false; eaten:= false; time_pass:= 0;
	gold_taken:= 0; freeze_c:= 0; cmd:= ' ';
	if diff = lv_max then cur_rec.mode:= 2
	else cur_rec.mode:= 1;

	with cur_rec do begin
		score:= 0; 
		clevn:= diff*3; cur_rec.slen:= (diff+1) div 2;
		sta_lv:= diff;
		if mode = 2 then writeln('WoW You are so brave!');
	end;
	writeln('You will now start with level ', diff);
	writeln('Prepare to start! @_@');
	flash(cont, cursor.x, cursor.y);

	placeobjects;
        refscr;	
	for s:='3' downto '1' do begin
		refhud(3, 'Start in ' + s + 's');
		for x:= 1 to 2 do begin
			delay(0.25, false);
			cursorto(player.x, player.y);
			write(char_t[empty_t]);
			delay(0.25, false);
			cursorto(player.x, player.y);
			write(char_t[player_t]);
		end;	{ flash the position of the player }
	end;
	refhud(3, 'Game Start!');
	sta_time:= gettickcount/1000;
end;	{ initialize }

procedure game_run;
var time_new, time_old, delay_time: real;
begin
	time_new:= 0;
	repeat
		time_old:= round(time_pass*10)/10;
		gettimepass;			{ refresh the time passed }
		if round(time_pass*10)/10 > time_old then refhud(1, '');

		{ time for next move }
		if  time_pass >= time_new then begin	
			if diff <= 10 then delay_time:= lv_time *(11 -diff)/10
			else delay_time:= lv_time /diff;
			time_new:= time_new + delay_time;

			playermove;		{ next move }
			if not eaten then if freeze_c = 0 then snakemove
			else begin
				dec(freeze_c);
				refhud(2, '');
			end;
		end;
	until eaten;
end;	{ game_run }

procedure game_end;
var new_score: boolean;
begin
	gettimepass;
	refscr;
	cursorto(0, height+2);
	writeln('Game over. The snake has eaten you. lol');
	with cur_rec do begin
		end_lv:= diff;
		time_p:= time_pass;
	end;
	new_score:= insert_data(cur_rec);
	repeat until chr(13) = flash(enter, 0, height +3);

	if new_score then begin
		cursorto(0, height+2); clreol;
		writeln('>>>>>>>>>> WoW You have entered the top 5! <<<<<<<<<<');
		flash(cont, 0, cursor.y);
		highscore;
	end;
end;	{ game_end }

(*************************************
*            main program            *
*************************************)
begin
	init;
	repeat
		game_inst(cmd);
		case cmd of
			'0'..'9': begin
				game_init;
				game_run;
				game_end;
			end;
			hscmd: highscore;
			chnamecmd: input_name;
		end;
	until cmd = esccmd;
	clrscr;
	flash('Thanks for playing. Goodbye. ^o^', 0, 0);
	clrscr;
end.	{ main prog }
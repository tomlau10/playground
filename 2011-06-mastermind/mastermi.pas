program MasterMind_v1_6;
uses wincrt;
type
    gus_rec = record                    {record type of past guesses}
                    guess: string[4];
                    corrguess: 0..4;
                    wrongplace: 0..4;
              end;
const
     max_gus = 20;                      {maximum times for guessing}          
var
   ans: string;                         {string for storing answer}
   gus: string;                         {string for storing guess}
   times: integer;                      {integer for storing guessed times}
   past_gus: array[1..20] of gus_rec;   {array for storing records of guesses}
   win: boolean;                        {true if one win the game}

procedure welcome;                      {display welcome message}
begin
     clrscr;
     writeln('Welcome to MasterMind');
     write('Press enter key to start.'); readln;
     clrscr;
     writeln('MasterMind now begin');
end; {welcome}

procedure instruction;
begin
     writeln('The answer is a 4 digit number formed by 0-9');
     writeln('That is 0000 - 9999');
     writeln('You can at most guess ',max_gus, ' times.');
     writeln('After ', max_gus, ' wrong guesses, correct answer will be shown.');
     writeln;
end; {instruction}

procedure random_ans;                   {program initialization}
begin
      ans:= '';                         {reset the strings}
      gus:= '';
      times:= 0;
      randomize;                        {random the answer and store it as string}
      str(random(10000), ans);
      while length(ans) < 4 do
            ans:= '0' + ans;
      (* writeln('ans: ', ans);	        {show the answer for testing} *)
end; {random_ans}

procedure show_gue(time: integer);      {display past guesses}
var i: integer;
begin
     for i:= 1 to time do begin
         with past_gus[i] do
         writeln('#', i:2, guess:5, ' corr:',corrguess:2, ' wrong pos:', wrongplace:2);
     end;
end; {show_gue}

procedure ck_input;                     {checking invalid input}
var
   num, code: integer;                  {int for converting input to num}
   valid: boolean;                      {boolean for valid input}
begin
     repeat
           instruction;
           show_gue(times-1);
           valid:= true;
           write('#', times: 2, ' Enter a 4-digit number: ');
           readln(gus); clrscr;
           val(gus, num, code);
           if (code <> 0) or (length(gus) <> 4) or (gus[1] in ['-', '+']) then begin
              valid:= false;
              writeln('Invalid input!');
              writeln;
           end;
     until valid;
end; {ck_input}

procedure ck_ans(var corr:integer);      {return the number of correct guesses}
var
   half: integer;                        {number of wrong position guesses}
   i, j: integer;                        {int for counting}
   ckd: array[1..2, 1..4] of boolean;    {string for storing the checked pos of ans/gus}
begin
     corr:= 0;                           {reset the num of corr guesses}
     half:= 0;
     for i:=1 to 2 do
         for j:=1 to 4 do ckd[i, j]:= false;

     for i:= 1 to 4 do                   {checking corr guesses}
         if (ans[i] = gus[i]) then begin
            corr:= corr + 1;
            for j:=1 to 2 do ckd[j, i]:= true;
         end;

     for i:= 1 to 4 do                   {checking wrong place guesses}
         for j:= 1 to 4 do
             if (ans[i] = gus[j]) and (not ckd[1, i]) and (not ckd[2, j]) then begin
                half:= half+1;
                ckd[1, i]:= true;
                ckd[2, j]:= true;
             end;

     with past_gus[times] do begin       {save the guess record}
          guess:= gus;
          corrguess:= corr;
          wrongplace:= half;
     end;
end; {ck_ans}

procedure start_guess;                   {start the guess}
var
   all_corr: integer;                    {equal to 4 if all correct}
begin
     repeat
           times:= times+1;
           ck_input;
           ck_ans(all_corr);
     until (all_corr = 4) or (times = max_gus);
     if all_corr = 4 then win:= true else win:= false;
end; {start_guess}

procedure show_win;                      {display winning or losing message}
begin
     show_gue(times);
     writeln;
     if win then begin
        writeln('You win! You guessed ', times, ' time(s) to get the answer.');
        writeln('And it is ', ans);
     end
     else begin
          writeln('I am sorry but you lost');
          writeln('The correct answer is ', ans);
     end;
     writeln;
end; {show_win}

function replay:boolean;                 {restart game checking}
var
   input: string;                        {string for storing input}
   valid: boolean;                       {boolean for valid input}
begin
     repeat
           valid:= true;
           write('Do you want to play again? (Y/N):');
           readln(input);
           if (length(input) <> 1) or not (input[1] in ['Y', 'y', 'N', 'n']) then begin
              clrscr;
              writeln('Invalid input!');
              writeln;
              show_win;
              valid:= false;
           end;
     until valid;
     if input[1] in ['Y', 'y'] then replay:= true else replay:= false
end; {replay}                                                  

{main program}
begin
     repeat
           welcome;
           random_ans;
           start_guess;
           show_win;
     until not replay;
     clrscr;
     writeln('Thanks for playing. Goodbye.')
end. {main program}
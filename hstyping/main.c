#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ncurses.h>
#include<signal.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/time.h>
#include<sys/types.h>
#include<aio.h>
#include<termios.h>
#include<sys/wait.h>

#define UPPEREDGE 2
#define LOWEREDGE 32
#define WINDOWSTART 40

int windowWidth;
int curs;   //cursor position
int inputRow;   //input row
int inputc; //input character
int cursStart;
int fileNum;
int isInitScreen;
int isMain;
int isGameScreen;
int isoutpage;
int isChild;
int done;
int donePro;
int outFlag;
int progExit;
int init;
int done;
int outChild; // child process's while loop flag
char title[30] = "HANSOL TYPING TRAINING";
char name[20];
char langList[3][50] = {"C", "JAVA", "PYTHON"};
char timerList[3][50] = {"1","3","5"};
char diffList[2][50] = {"NORMAL", "HARD"};
char lang[20];
char timer[20];
char diff[20];
char blank[200] = "                                                                                                                                                     ";
FILE* fp[9];
char buf[5][201];
char pass[201] = "                        ";
int start = 0, end = 4;
struct aiocb kbcbuf;	

int numSentence;
int remain_time;
int typing_time;
int first_typing;
int max_rate;
int aver_rate;
int cur_rate;
int acc_rate;
int numTyped;
int backTyped;
int len_of_curstr;
int pipe1[2];
int pipe2[2];
int pipe3[2];

struct UI{
    int row; // start row
    int col; // start col
    int sen[6];
    int hdcol1, hdcol2, hdcol3; 
    int sccol1, sccol2, sccol3;
};
struct UI ui;

int setticker(int n){
    struct itimerval new_timeset;
	long    n_sec, n_usecs;
	n_sec = n / 1000 ;		/* int part*/
	n_usecs = ( n % 1000 ) * 1000L ;  /* remainder*/

	new_timeset.it_interval.tv_sec = n_sec;		/* set reload */
	new_timeset.it_interval.tv_usec = n_usecs;  /* new ticker value */
	new_timeset.it_value.tv_sec     = n_sec;	/* store this */
	new_timeset.it_value.tv_usec    = n_usecs;	/* and this */

	return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

void displayBorder(){
    for(int i=WINDOWSTART; i<COLS - WINDOWSTART; i++){
        mvaddch(UPPEREDGE, i, '=');
        mvaddch(LOWEREDGE, i, '=');
    }
}

void wrapup(){
    clear();
    curs_set(1);
    endwin();
    for(int i=0; i<9; i++){
        if (i != 2 && i != 5 && i!= 8)
            fclose(fp[i]);
    }
}

int nextIdx(int idx){
    if (idx == 4)
        return 0;
    else 
        return idx+1;
}

void nextLine(){
    int idx;
    if (init == 0){
        fscanf(fp[fileNum], "%[^\n] ", buf[0]);
        fscanf(fp[fileNum], "%[^\n] ", buf[1]);
        fscanf(fp[fileNum], "%[^\n] ", buf[2]);
        fscanf(fp[fileNum], "%[^\n] ", buf[3]);
        fscanf(fp[fileNum], "%[^\n] ", buf[4]);
        init = 1;
        start = 0;
        end = 4;
    }
    else{
        strcpy(pass, buf[start]);
        start = nextIdx(start);
        end = nextIdx(end);
        if (fscanf(fp[fileNum], "%[^\n] ", buf[end]) == EOF){
            strcpy(buf[end],blank);
        }
    }
    
    if (strcmp(buf[start],blank) == 0){
        outChild = 1;
        return;
    } 
    mvaddstr(ui.row+2, ui.sccol1, "     ");
    move(ui.row+2, ui.sccol1);
    attron(COLOR_PAIR(2));
    printw("%d", cur_rate);
    attron(COLOR_PAIR(1));

    mvaddstr(ui.row+2, ui.sccol2, "     ");
    move(ui.row+2, ui.sccol2);
    attron(COLOR_PAIR(2));
    printw("%d", aver_rate);
    attron(COLOR_PAIR(1));
    mvaddstr(ui.row+2, ui.sccol3, "     ");
    move(ui.row+2, ui.sccol3);
    attron(COLOR_PAIR(2));
    printw("%d%%", acc_rate);
    attron(COLOR_PAIR(1));

    len_of_curstr = strlen(buf[start]);
    idx = start;
    mvaddstr(ui.sen[0], cursStart, blank);
    mvaddstr(ui.sen[0], cursStart, pass);
    for(int i=1; i<6; i++){
        mvaddstr(ui.sen[i], cursStart, blank);
        mvaddstr(ui.sen[i], cursStart, buf[idx]);
        idx = nextIdx(idx);
    }
    move(inputRow, curs);
    refresh();
}

void on_input_exit(int snum){
    char *cp = (char *) kbcbuf.aio_buf;	
    int c;
	if ( aio_error(&kbcbuf) != 0 )
		perror("reading failed");
	else {
		if ( aio_return(&kbcbuf) == 1 )
		{   
            c = *cp;
            if (c == 13){
                done = 1;
            }
        }
    }
    aio_read(&kbcbuf);
}

void on_input_game(int snum)
{	
	char *cp = (char *) kbcbuf.aio_buf;	
    int senc, x, y;
	if ( aio_error(&kbcbuf) != 0 )
		perror("reading failed");
	else 
		if ( aio_return(&kbcbuf) == 1 )
		{
			inputc = *cp;
            if (!first_typing){
                first_typing = 1;
            }
			if ( inputc == 127 && curs > cursStart){     //backspace key
                senc = mvinch(inputRow-2,curs-1) & A_CHARTEXT;
                if (curs != cursStart && senc != (mvinch(inputRow, curs-1) & A_CHARTEXT)){
                    attron(COLOR_PAIR(1));
                    mvaddch(inputRow-2, curs-1, senc);
                }
                else {
                    if (numTyped != 0)
                        numTyped--;
                }
                
            
                mvaddch(inputRow,curs-1,' ');
                curs--;
                move(inputRow, curs);
                backTyped++;
            }
			else if ( inputc == 13 ){     //enter key
                mvaddstr(inputRow,cursStart,blank);
                curs = cursStart;
                cur_rate = (int)((numTyped - backTyped)/(double)typing_time*60);
                if (cur_rate < 0) 
                    cur_rate = 0;
                aver_rate = (int)((aver_rate*numSentence + cur_rate)/(double)(numSentence+1));
                acc_rate = (int)((acc_rate*numSentence + numTyped/(double)(len_of_curstr-1)*100) / (double)(++numSentence));
                if (cur_rate > max_rate)
                    max_rate = cur_rate;
                nextLine();
                first_typing = 0;
                numTyped = 0;
                backTyped = 0;
                typing_time = 0;
            }
            else{
                if (!(curs == cursStart && inputc==127)){
                    mvaddch(inputRow,curs,inputc);
                    curs++;
                    move(inputRow, curs);
                    x = getcurx(stdscr);
                    y = getcury(stdscr);    
                    senc = mvinch(inputRow-2,curs-1) & A_CHARTEXT;
                    if (curs != cursStart && senc != (mvinch(inputRow, curs-1) & A_CHARTEXT)){
                        attron(COLOR_PAIR(4));
                        mvaddch(inputRow-2, curs-1, senc);
                        attron(COLOR_PAIR(1));
                    }
                    else{
                        attron(COLOR_PAIR(1));
                        mvaddch(inputRow-2, curs-1, senc);
                        numTyped++;
                    }
                    move(y,x);
                }
            }
            refresh();
		}
	aio_read(&kbcbuf);
}

void setup_aio_buffer()
{
	static char input[1];		      
	kbcbuf.aio_fildes     = 0;	     
	kbcbuf.aio_buf        = input;	      
	kbcbuf.aio_nbytes     = 1;             
	kbcbuf.aio_offset     = 0;            
	kbcbuf.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	kbcbuf.aio_sigevent.sigev_signo  = SIGIO;  
}

void on_input(int signum){
    char *cp = (char *) kbcbuf.aio_buf;	
    int c, x, y;
	if ( aio_error(&kbcbuf) != 0 )
		perror("reading failed");
	else {
		if ( aio_return(&kbcbuf) == 1 )
		{
			c = *cp;
            if (c == 'y'){
                done = 1;
                outFlag = 1;
            }
            if (c== 'n'){
                done = 1;
                outFlag = 0;
            }
        }
    }
}

void fetchFile(){
    fp[0] = fopen("text_file/c_normal.txt","r");
    fp[1] = fopen("text_file/c_hard.txt","r");
    
    fp[3] = fopen("text_file/java_normal.txt","r");
    fp[4] = fopen("text_file/java_hard.txt","r");
    
    fp[6] = fopen("text_file/py_normal.txt","r");
    fp[7] = fopen("text_file/py_hard.txt","r");
}


void blinkstr1(int signum){
    static int flag = 1;
    char anyKey[50] = "  Press any key to start  ";
    int messLen = strlen(anyKey);
    if (flag){
        attron(A_STANDOUT);
        mvaddstr(LOWEREDGE-12, WINDOWSTART + windowWidth/2 - messLen/2, anyKey);
        attroff(A_STANDOUT);
    }
    else {
        mvaddstr(LOWEREDGE-12, WINDOWSTART + windowWidth/2 - messLen/2, blank);
    }
    refresh();
    flag = !flag;
}

void blinkstr2(int signum){
    static int flag = 1;
    char anyKey[50] = "  Press any key to practice  ";
    int messLen = strlen(anyKey);
    int num = 6, timeLine = num+14;
    if (flag){
        attron(A_STANDOUT);
        mvaddstr(timeLine+8, WINDOWSTART + windowWidth/2 - strlen(anyKey)/2, anyKey);
        attroff(A_STANDOUT);
    }
    else {
        mvaddstr(timeLine+8, WINDOWSTART + windowWidth/2 - strlen(anyKey)/2, blank);
    }
    refresh();
    flag = !flag;
}

void blinkstr3(int signum){
    static int flag = 1;
    char anyKey[50] = "  Press 'enter' to exit  ";
    int messLen = strlen(anyKey);
    int num = 6, timeLine = num+14;
    if (flag){
        attron(A_STANDOUT);
        mvaddstr(25, WINDOWSTART + windowWidth/2 - strlen(anyKey)/2, anyKey);
        attroff(A_STANDOUT);
    }
    else {
        mvaddstr(25, WINDOWSTART + windowWidth/2 - strlen(anyKey)/2, blank);
    }
    refresh();
    flag = !flag;
}


void childTimer(int signum){
    int x, y;
    if (first_typing){
        typing_time++;
    }
    if (remain_time-- == 0){
        outChild = 1;
    }
    else {
        x = getcurx(stdscr);
        y = getcury(stdscr);    
        mvaddstr(ui.row+2, WINDOWSTART + windowWidth - 10, "          ");
        move(ui.row+2, WINDOWSTART + windowWidth - 8);
        attron(COLOR_PAIR(5));
        printw("%d", remain_time);
        attron(COLOR_PAIR(1));
        move(y,x);
        refresh();
    }    
}
void outPage(int signum){
    WINDOW* w;
    int width = 25, height = 1;
    int c, x, y;
    int pid;
    signal(SIGIO, on_input);
    signal(SIGALRM, SIG_IGN);
    x = getcurx(stdscr);
    y = getcury(stdscr);
    w = newwin(height,width,UPPEREDGE+1,130);
    overwrite(w, curscr);
    wattron(w,COLOR_PAIR(2));
    mvwprintw(w,0,1," Sure to exit? (y/n)");
    wrefresh(w);
    if (!isChild)
        aio_read(&kbcbuf);
    while(!done)
        pause();
    done = 0;
    
    werase(w);
    wrefresh(w);
    move(y,x);
    refresh();
    
    if (isChild){
        if (outFlag){
            outFlag = 0;
            exit(1);
        }
        signal(SIGIO, on_input_game);
        signal(SIGALRM, childTimer);
        setticker(1000);
        aio_read(&kbcbuf);	
    }
    else{
        if (outFlag){
            outFlag = 0;
            wrapup();
            exit(1);
        }
        signal(SIGIO, SIG_IGN);  
        if (isInitScreen){
            signal(SIGALRM, blinkstr1);
            setticker(800);
        }
    }
}

void enterMain(){
    int c, out;
    char err[30] = "Wrong Input";
    char langMenu[50] = "/*  (0) C   (1) JAVA   (2) PYTHON  */";
    char diffMenu[50] = "/*  (0) NORMAL   (1) HARD  */";
    char timeMenu[50] = "/*  (0) 1Min   (1) 3Min   (2) 5Min  */";
    int col = WINDOWSTART + 3;

    int num = UPPEREDGE + 6;
    int nameLine = num, langLine = num+4, diffLine = num+9, timeLine = num+14;
    clear();
    isMain = 1;
    displayBorder();
    mvaddstr(UPPEREDGE+1, WINDOWSTART,"  Main  |");
    mvaddstr(UPPEREDGE+1, WINDOWSTART + windowWidth/2-strlen(title)/2, title);
    for(int i=WINDOWSTART; i<COLS - WINDOWSTART; i++)
        mvaddch(UPPEREDGE+2,i,'-');
    
    mvaddstr(nameLine,col,"User Name: ");
    scanw("%s",name);
    move(nameLine+1, col);
    attron(COLOR_PAIR(5));
    printw(">> Hi, %s !", name);
    attron(COLOR_PAIR(1));
    refresh();
    //language menu
    usleep(600000);
    out = 0;
    attron(COLOR_PAIR(6));
    mvaddstr(langLine,col, langMenu);
    attron(COLOR_PAIR(1));
    mvaddstr(langLine+1,col, "Choose Your Programming Language [0~2]: ");
    while(!out){
        scanw("%s%*c",lang);
        mvaddstr(langLine+2,0,blank);
        switch(*lang){
            case '0':
                out = 1;
                break;  
            case '1':
                out = 1;
                break;
            case '2':
                out = 1;
                break;                
            default:
                attron(COLOR_PAIR(4));
                mvaddstr(langLine+2, col + strlen("Choose Your Programming Language [0~2]: "), err);
                attron(COLOR_PAIR(1));
                mvaddstr(langLine+1, col + strlen("Choose Your Programming Language [0~2]: "),blank);
                move(langLine+1, col + strlen("Choose Your Programming Language [0~2]: "));
                break;
        }
    }
    move(langLine+2, col);
    attron(COLOR_PAIR(5));
    printw(">> You chose '%s' !", langList[*lang-'0']);
    attron(COLOR_PAIR(1));
    refresh();
    //difficulty menu
    usleep(600000);
    out = 0;
    attron(COLOR_PAIR(6));
    mvaddstr(diffLine,col, diffMenu);
    attron(COLOR_PAIR(1));
    mvaddstr(diffLine+1,col, "Choose Difficulty [0~1]: ");
    while(!out){
        scanw("%s%*c",diff);
        mvaddstr(diffLine+2,0,blank);
        switch(*diff){
            case '0':
                out = 1;
                break;  
            case '1':
                out = 1;
                break;
            default:
                attron(COLOR_PAIR(4));
                mvaddstr(diffLine+2, col + strlen("Choose Difficulty [0~1]: "), err);
                attron(COLOR_PAIR(1));
                mvaddstr(diffLine+1, col + strlen("Choose Difficulty [0~1]: "),blank);
                move(diffLine+1, col + strlen("Choose Difficulty [0~1]: "));
                break;
        }
    }
    
    move(diffLine+2,col);
    attron(COLOR_PAIR(5));
    printw(">> You chose '%s' mode !", diffList[(*diff-'0')]);
    attron(COLOR_PAIR(1));
    refresh();
    //timer menu
    usleep(600000);
    out = 0;
    attron(COLOR_PAIR(6));
    mvaddstr(timeLine,col, timeMenu);
    attron(COLOR_PAIR(1));
    mvaddstr(timeLine+1,col, "Choose Timer [0~2]: ");
    while(!out){
        scanw("%s%*c",timer);
        mvaddstr(timeLine+2,0,blank);
        switch(*timer){
            case '0':
                out = 1;
                break;  
            case '1':
                out = 1;
                break;
            case '2':
                out = 1;
                break;
            default:
                attron(COLOR_PAIR(4));
                mvaddstr(timeLine+2, col + strlen("Choose Timer [0~2]: "), err);
                attron(COLOR_PAIR(1));
                mvaddstr(timeLine+1, col + strlen("Choose Timer [0~2]: "),blank);
                move(timeLine+1, col + strlen("Choose Timer [0~2]: "));
                break;
        }
    }
    
    //press any key
    move(timeLine+2,col);
    attron(COLOR_PAIR(5));
    printw(">> You chose '%s minute' !", timerList[(*timer-'0')]);
    attron(COLOR_PAIR(1));
    refresh();
    curs_set(0);
    signal(SIGALRM, blinkstr2);
    setticker(800);
    isMain = 0;
    isGameScreen = 1;
    
    getch();

    signal(SIGALRM, SIG_IGN);
    curs_set(1);
}

void child(){
    sigset_t block;
    char div[100] = ".....................................................................................";
    char scoreLine[100] = "*************************************************************************************";
    char gap[10] = "   =>  ";
    char temp[201];
    char hdStr[4][100] = {"", "Current Typing Rate", "Average Typing Rate", "Accuracy"};
    isChild = 1;
    clear();
    //UI
    ui.row = UPPEREDGE + 5;
    ui.col = WINDOWSTART + windowWidth/2 - strlen(scoreLine)/2;
    ui.sen[0] = ui.row+6; ui.sen[1] = ui.row+9; ui.sen[2] = ui.row+16;
    ui.sen[3] = ui.row+17; ui.sen[4] = ui.row+18; ui.sen[5] = ui.row+19;
    ui.hdcol1 = ui.col + strlen(scoreLine)/3/2 - strlen(hdStr[1])/2; 
    ui.hdcol2 = ui.col + strlen(scoreLine)/3 + strlen(scoreLine)/3/2 - strlen(hdStr[2])/2; ; 
    ui.hdcol3 = ui.col + strlen(scoreLine)/3*2 + strlen(scoreLine)/3/2 - strlen(hdStr[3])/2; ; 
    ui.sccol1 = ui.col + strlen(scoreLine)/3/2 - 2;
    ui.sccol2 = ui.col + strlen(scoreLine)/3 + strlen(scoreLine)/3/2 - 2;
    ui.sccol3 = ui.col + strlen(scoreLine)/3*2 + strlen(scoreLine)/3/2 - 2;
    displayBorder();
    mvaddstr(UPPEREDGE + 1, WINDOWSTART,"  Practice  |  ");
    mvaddstr(UPPEREDGE + 1, WINDOWSTART + windowWidth/2 - strlen(title)/2, title);
    for(int i=WINDOWSTART; i<COLS - WINDOWSTART; i++)
        mvaddch(UPPEREDGE + 2,i,'-');
    start_color();     	 
    //first score line
    mvaddstr(ui.row, ui.col, scoreLine);
    mvaddstr(ui.row+3, ui.col, scoreLine);
    mvaddstr(ui.row+1, ui.hdcol1, hdStr[1]);
    mvaddch(ui.row+1, ui.col + strlen(scoreLine)/3, '|');
    mvaddch(ui.row+2, ui.col + strlen(scoreLine)/3, '|');
    mvaddstr(ui.row+1, ui.hdcol2, hdStr[2]);
    mvaddch(ui.row+1, ui.col + strlen(scoreLine)/3*2, '|');
    mvaddch(ui.row+2, ui.col + strlen(scoreLine)/3*2, '|');
    mvaddstr(ui.row+1, ui.hdcol3, hdStr[3]);
    mvaddstr(ui.row+1, WINDOWSTART + windowWidth - 9, "Time");

    mvaddstr(ui.row+5, ui.col, div);
    attron(COLOR_PAIR(5));
    mvaddstr(ui.sen[0], ui.col + 1, "pass");
    attron(COLOR_PAIR(1));
    mvaddstr(ui.row+7, ui.col, div);
    attron(COLOR_PAIR(6));
    mvaddstr(ui.row+11, ui.col, gap);
     attron(COLOR_PAIR(1));
    mvaddstr(ui.row+13, ui.col, div);
    attron(COLOR_PAIR(5));
    mvaddstr(ui.sen[2]-1, ui.col + 1, "next");
    attron(COLOR_PAIR(1));
    mvaddstr(ui.row+21, ui.col, div);
    refresh();

    remain_time = ((*timer - '0' + 1)*2 - 1)*60;
    signal(SIGALRM, childTimer);
    setticker(1000);

    //input String
    signal(SIGIO, on_input_game);  
    fileNum = (*lang-'0')*3 + (*diff-'0'); // 0, 1, 3, 4, 6, 7
    inputRow = ui.row+11;
    cursStart = ui.col + strlen(gap);
    init = 0;
    curs = cursStart;
    nextLine();
    aio_read(&kbcbuf);	
	
    while(!outChild){    // file EOF or timer end >> exit
		pause();
    }
    sprintf(temp,"%d",max_rate);
    write(pipe1[1], temp, strlen(temp));
    strcpy(temp, blank);

    sprintf(temp,"%d",aver_rate);
    write(pipe2[1], temp, strlen(temp));
    strcpy(temp, blank);

    sprintf(temp,"%d",acc_rate);
    write(pipe3[1], temp, strlen(temp));

    isChild = 0;
    signal(SIGALRM, SIG_IGN);

    exit(0);
}

void gameStart(){
    int pid;
    int status, exitVal;
    char under[50] = "--------------------------------";
    int col = WINDOWSTART + windowWidth/2 - strlen(under)/2, row = UPPEREDGE + 12;
    int max, aver, acc;
    char temp[201];
    clear();
    displayBorder();
    mvaddstr(UPPEREDGE+1,WINDOWSTART, "  Result  |");
    mvaddstr(UPPEREDGE+1, COLS/2-strlen(title)/2, title);
    for(int i=WINDOWSTART; i<COLS - WINDOWSTART; i++)
        mvaddch(UPPEREDGE+2,i,'-');

    pipe(pipe1);
    pipe(pipe2);
    pipe(pipe3);
    pid = fork();
    if (pid == 0){
        child();
    }
    else{
        signal(SIGINT,SIG_IGN);
        wait(&status);
        exitVal = status >> 8;
        if (exitVal == 1){
            progExit = 1;
            signal(SIGINT, outPage);
            return;
        }
        
        read(pipe1[0],temp,10);
        max = atoi(temp);
        strcpy(temp,blank);

        read(pipe2[0],temp,10);
        aver = atoi(temp);
        strcpy(temp,blank);
        
        read(pipe3[0],temp,10);
        acc = atoi(temp);

        attron(COLOR_PAIR(5));
        mvaddstr(row,col+1,name);   
        attron(COLOR_PAIR(1));
        mvaddstr(row,col+1+strlen(name),"'s practice result !");
        mvaddstr(row+1,col, under);

        move(row+3, col+2);
        printw("Max Typing Rate: ");
        attron(COLOR_PAIR(2));
        printw("%d", max);
        attron(COLOR_PAIR(1));

        move(row+5, col+2);
        printw("Average Typing Rate: ");
        attron(COLOR_PAIR(2));
        printw("%d", aver);
        attron(COLOR_PAIR(1));

        move(row+7, col+2);
        printw("Accuracy: ");
        attron(COLOR_PAIR(2));
        printw("%d%%", acc);
        attron(COLOR_PAIR(1));
        refresh();
        curs_set(0);
        signal(SIGIO, SIG_IGN);
        sleep(1);
        signal(SIGALRM, blinkstr3);
        setticker(800);
        signal(SIGIO, on_input_exit);
        aio_read(&kbcbuf);
        while(!done)
            pause();
        done = 0;
        curs_set(1);
        progExit = 1;
    }
}

void start_prog(){
    char title[30] = "HANSOL TYPING TRAINING";
    int titleLen = strlen(title);
    clear();
    curs_set(0);
    displayBorder();
    attron(A_BOLD | A_UNDERLINE);
    mvaddstr(LOWEREDGE/2-1, WINDOWSTART + windowWidth/2 - titleLen/2, title);
    attroff(A_UNDERLINE);
    signal(SIGALRM, blinkstr1);
    setticker(800);
    getch();
    isChild = 0;
    isInitScreen = 0;
    isMain = 1;
    done = 0;
    curs_set(1);
    signal(SIGALRM, SIG_IGN);
}

void setup(){
    fetchFile();
    setup_aio_buffer();	  
    initscr();
    start_color();
    signal(SIGINT, outPage);
    init_pair(1,COLOR_WHITE, COLOR_BLACK);
    init_pair(2,COLOR_YELLOW, COLOR_BLACK);
    init_pair(3,COLOR_BLACK, COLOR_WHITE);
    init_pair(4,COLOR_RED, COLOR_BLACK);
    init_pair(5,COLOR_CYAN, COLOR_BLACK);
    init_pair(6,COLOR_BLUE, COLOR_BLACK);
    windowWidth = COLS - WINDOWSTART*2;
    bkgd(COLOR_PAIR(1));
    cbreak();
    isInitScreen = 1;
    isMain = 0;
    isGameScreen = 0;
}

int main(){
    setup();
    while(!progExit){
        if (isInitScreen)
            start_prog();
        if (isMain)
            enterMain();
        if (isGameScreen)
            gameStart();
    }
    wrapup();

    return 0;
}
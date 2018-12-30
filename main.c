#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAB_SIZE 4
#define KEY_CTRL 0x1f
#define KEY_BS 127
#define KEY_ENTR 10

typedef struct {
    char* content;
    int currentDataSize;
    int currentUsableSize;
    int cursorPos;
} textData;

char* validAlphabet = "\t 1234567890QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm!\"£$%^&*()-_=+[]{};:@'~#,<.>/?\\|";

void initNcurses() {
	initscr(); 
    noecho(); // Don't output user input 
    raw(); // Ignore ctrl-C, take only raw input
    start_color();
    init_color(COLOR_BLACK, 0, 0, 0);
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    keypad(stdscr, 1);
    cbreak(); 
}


/*
 * TODO:
 * - Is splitting the entire file by newline
 *   inefficient?? try on a huge file
 */
void updateScreen(int ch, textData* data) {
	clear();
    int screenh, screenw;
    getmaxyx(stdscr, screenh, screenw);

    //print tildes
    {
        attron(COLOR_PAIR(2));
        for(int i = 0; i < screenh - 2; i++) {
            mvprintw(i, 0, "~");
        }
        attroff(COLOR_PAIR(2));
    }

    //status bar
    {
        attron(COLOR_PAIR(1));
        for (int i = 0; i < screenw; ++i){
            mvprintw(screenh-2,i," ");
        }
        mvprintw(screenh-2,1,
            "CHAR:%s [%i], size:%i usable:%i", 
            keyname(ch), ch, data->currentDataSize, data->currentUsableSize);
        attroff(COLOR_PAIR(1));
    }

    // print data contents, split by newline
    {
        int current = 0;
        int currentRow = 0;
        int currentCol = 0;
        while(data->content[current] != '\0'){
            if(data->content[current] == '\n'){
                currentCol = 0;
                currentRow++;
            }else{
                mvprintw(currentRow, currentCol + 3, "%c", data->content[current]);
                if(data->content[current] == '\t'){
                    currentCol += TAB_SIZE;
                }else{
                    currentCol++;
                }
            }
            current++;
        }

        // Print line numbers
        attron(COLOR_PAIR(3));
        for(int i=0; i <= currentRow; i++) {
            mvprintw(i, 0, "%i", i+1);
        }
        attroff(COLOR_PAIR(3));
    }

    // Move cursor to valid position
    {
        int cursorRow = 0;
        int cursorCol = 0;
        int current = 0;
        for(int i = 0; i < data->cursorPos; i++){
            if(data->content[i] == '\n') {
                cursorRow++;
                cursorCol = 0;
            }else{
                if(data->content[i] == '\t'){
                    cursorCol += TAB_SIZE;
                }else{
                    cursorCol++;
                }
            }
        }
        // mvprintw(10, 30, "[cR, cC, cP]: %i %i, %i", cursorRow, cursorCol, data->cursorPos);
        move(cursorRow, cursorCol + 3);
    }

    refresh();
}

void deleteAtIndex(textData* data, int idxToDel, int moveBack) {
    if(idxToDel >= 0 && idxToDel < data->currentDataSize){
        memmove(&data->content[idxToDel], &data->content[idxToDel + 1], strlen(data->content) - idxToDel);
        data->currentDataSize--;
        if(data->cursorPos>0 && moveBack == 1) data->cursorPos--;
    }
}

bool cursorValidAndNotOnNewline(textData* data) {
    return data->content[data->cursorPos-1] != '\n' && 
            data->cursorPos <= data->currentDataSize &&
            data->cursorPos > 0;
}

void moveCursorDown(textData* data) {
    //get distance from left
    int distanceFromLeft = 0;
    if(data->cursorPos >= data->currentDataSize) return;

    while(cursorValidAndNotOnNewline(data)) {
        distanceFromLeft++;
        data->cursorPos--;
    }
    //go to next line
    data->cursorPos++;
    while(cursorValidAndNotOnNewline(data)){
        data->cursorPos++;
    }
    //move same distance
    for(int i = 0; i < distanceFromLeft; i++){
        data->cursorPos++;
        if(cursorValidAndNotOnNewline(data)){
            data->cursorPos++;
        }
        data->cursorPos--;
    }
    if(data->cursorPos > data->currentDataSize) { data->cursorPos--; }
}

void moveCursorUp(textData* data) {
    //get distance from left
    int distanceFromLeft = 0;
    if(data->cursorPos <= 0) return;

    while(cursorValidAndNotOnNewline(data)) {
        distanceFromLeft++;
        data->cursorPos--;
    }
    //go to previous line
    data->cursorPos--;
    while(cursorValidAndNotOnNewline(data)){
        data->cursorPos--;
    }
    //move same distance
    for(int i = 0; i < distanceFromLeft; i++){
        data->cursorPos++;
        if(cursorValidAndNotOnNewline(data)){
            data->cursorPos++;
        }
        data->cursorPos--;
    }
    if(data->cursorPos < 0) { data->cursorPos++; }
}

void insertCharAtPos(textData* data, char ch) {
    //Assuming character is not a command
    if(data->currentDataSize >= data->currentUsableSize) {
        data->content = 
            realloc(data->content, strlen(data->content) + 50 * sizeof(char));
        data->currentUsableSize += 50;
    }
    data->content[data->currentDataSize] = ch;
    
    memmove(&data->content[data->cursorPos], &data->content[data->cursorPos - 1], strlen(data->content) - data->cursorPos);
    data->content[data->cursorPos] = ch;
    data->currentDataSize++;
    data->cursorPos++;
}

void eventLoop(textData* data) {
    // Event management here
    int quit = 0;
    while(quit == 0){
        int ch = wgetch(stdscr);
        switch(ch) {
            case 'q' & KEY_CTRL: quit = 1; break;
            case KEY_BS: deleteAtIndex(data, data->cursorPos - 1, 1); break;
            case KEY_DC: deleteAtIndex(data, data->cursorPos, 0); break;

            // ctrl+backspace removes 
            case KEY_BACKSPACE:
                if(data->content[data->cursorPos-1] == ' ' || data->content[data->cursorPos-1] == '\n') {
                    deleteAtIndex(data, data->cursorPos-1, 1);
                }else{
                    while(data->content[data->cursorPos-1] != ' ' &&
                      data->content[data->cursorPos-1] != '\n' &&
                      data->currentDataSize > 0 &&
                      data->cursorPos > 0) {
                        deleteAtIndex(data, data->cursorPos-1, 1);
                    }
                }
            break;

            case KEY_LEFT: if(data->cursorPos > 0) data->cursorPos--; break;
            case KEY_RIGHT: if(data->cursorPos < data->currentDataSize) data->cursorPos++; break;
            case KEY_DOWN: moveCursorDown(data); break;
            case KEY_UP: moveCursorUp(data); break;

            // ENTER
            case KEY_ENTR: insertCharAtPos(data, '\n'); break;

            // CTRL+LEFT
            case 553:
                if(data->content[data->cursorPos-1] == ' ' || data->content[data->cursorPos-1] == '\n') {
                    data->cursorPos -= 1;
                }else{
                    while(data->content[data->cursorPos-1] != ' ' && cursorValidAndNotOnNewline(data)) {
                        data->cursorPos -= 1;
                    }
                }
            break;

            // CTRL+RIGHT
            case 568:
                if(data->content[data->cursorPos] == ' ' || data->content[data->cursorPos] == '\n') {
                    data->cursorPos += 1;
                }else{
                    data->cursorPos++;
                    while(data->content[data->cursorPos-1] != ' ' && cursorValidAndNotOnNewline(data)) {
                        data->cursorPos += 1;
                    }
                    data->cursorPos--;
                }
            break;

            default: if(strchr(validAlphabet, (char)ch) != NULL) insertCharAtPos(data, ch); break;
        }
        updateScreen(ch, data);
    }
}

int main() {
	initNcurses();
	
    textData data;
    	data.content = malloc(sizeof(char) * 50);
        data.currentUsableSize = 50;
        data.currentDataSize = 0;
        data.cursorPos = 0;

    strcpy(data.content, "");
	
    updateScreen(0, &data);
	move(0, 3);
    eventLoop(&data);

	endwin();
    free(data.content);
	return 0;
}
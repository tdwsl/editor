/*
 * This is my atttempt at a text editor
 * in C.
 *
 * I edit a lot of text, mostly on the
 * commandline, so I figured it would be
 * nice if I could make my own editor.
 *
 * Feel free to share.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <stddef.h>
#include <string.h>

char *text, *filename = 0;
int cx = 0, cy = 0;
int yscroll = 0, xscroll = 0;

void freeText() {
    free(text);
}

int getCursorPos() {
    int l = strlen(text);
    int x = 0, y = 0;
    int p;
    for(p = 0; p < l; p++) {
        if(y == cy && x == cx)
            break;
        if(y > cy)
            break;
        x++;
        if(text[p] == '\n') {
            y++;
            x = 0;
        }
    }
    if(x < cx)
        cx = x;
    if(y < cy) {
        cy = y;
        p = l-1;
    }
    return p;
}

void setCursorPos(int p) {
    cx = 0;
    cy = 0;
    for(int i = 0; i < p; i++) {
        cx++;
        if(text[i] == '\n') {
            cx = 0;
            cy++;
        }
    }
}

void insertChar(char c) {
    int p = getCursorPos();
    int l = strlen(text);
    for(int i = l; i > p; i--)
        text[i] = text[i-1];
    text[p] = c;
    cx++;
    if(c == '\n') {
        cx = 0;
        cy++;
    }
}

void insertTab() {
    int w = 4 - cx%4;
    int l = strlen(text);
    int p = getCursorPos();
    for(int i = l+w; i >= p+w; i--)
        text[i] = text[i-w];
    for(int i = p; i < p+w; i++)
        text[i] = ' ';
    cx += w;
}
    
void deleteChar() {
    int p = getCursorPos();
    for(int i = p; text[i]; i++)
        text[i] = text[i+1];
}

void loadFile(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if(!fp)
        return;
    char buf[256];
    int l = 0;
    while(fgets(buf, 256, fp)) {
        text = realloc(text, strlen(text)+256);
        sprintf(text, "%s%s", text, buf);
    }
    fclose(fp);
}

void initCurses() {
    initscr();
    keypad(stdscr, 1);
}

void endCurses() {
    keypad(stdscr, 0);
    endwin();
}

void draw() {
    int w, h;
    getmaxyx(stdscr, h, w);
    int yo = 1;
    int xo = 0;
    clear();
    int x = 0, y = 0;
    int l = strlen(text);
    for(int i = 0; y+yo-yscroll < h && i < l; i++) {
        mvaddch(y+yo-yscroll, x+xo-xscroll, text[i]);
        x++;
        if(text[i] == '\n') {
            x = 0;
            y++;
        }
    }
    attron(A_STANDOUT);
    for(int x = 0; x < w; x++)
        mvaddch(0, x, ' ');
    char coords[100];
    sprintf(coords, "%d,%d", cy, cx);
    mvaddstr(0, w-strlen(coords)-1, coords);
    const char *editingMsg = "Editing: ";
    char *nofileMsg = "[UNNAMED_FILE]";
    mvaddstr(0, 1, editingMsg);
    char *name = filename;
    if(!name)
        name = nofileMsg;
    mvaddstr(0, 1+strlen(editingMsg), name);
    attrset(A_NORMAL);
    move(cy+yo-yscroll, cx+xo-xscroll);
    refresh();
}

void input() {
    wchar_t c = getch();
    int p;
    switch(c) {
    case KEY_UP:
        if(cy > 0)
            cy--;
        getCursorPos();
        break;
    case KEY_DOWN:
        cy++;
        getCursorPos();
        break;
    case KEY_LEFT:
        if(cx > 0)
            cx--;
        else {
            p = getCursorPos();
            if(p > 0)
                setCursorPos(p-1);
        }
        break;
    case KEY_RIGHT:
        p = getCursorPos();
        if(text[p+1] != '\n') {
            if(text[p] != 0)
                cx++;
        }
        else {
            cy++;
            cx = 0;
        }
        break;
    case KEY_BACKSPACE:
        if(cx > 0) {
            cx--;
            deleteChar();
        }
        else {
            p = getCursorPos();
            if(p > 0) {
                setCursorPos(p-1);
                deleteChar();
            }
        }
        break;
    case 0x09:
        insertTab();
        break;
    default:
        if((c < 0x20 && c != '\n') || c == 0x7f || c >= 0xff)
            break;
        insertChar((char)c);
        break;
    }
    
    if(strlen(text)+20 > sizeof(*text)) {
        text = realloc(text, strlen(text)+20);
        for(int i = strlen(text); i < strlen(text)+20; i++)
            text[i] = 0;
    }
    int w, h;
    getmaxyx(stdscr, h, w);
    if(cy-yscroll > h-10)
        yscroll += 1;
    if(cy-yscroll < 10)
        yscroll -= 1;
    if(yscroll < 0)
        yscroll = 0;
    if(cx-xscroll > w-6)
        xscroll += 1;
    if(cx-xscroll < 7)
        xscroll -= 1;
    if(xscroll < 0)
        xscroll = 0;
}

int main(int argc, char **args) {
    initCurses();
    text = calloc(1, 100);
    if(argc > 1) {
        filename = args[1];
        FILE *fp = fopen(filename, "r");
        if(fp) {
            fclose(fp);
            loadFile(filename);
        }
    }
    for(;;) {
        draw();
        input();
    }
    free(text);
    endCurses();
    return 0;
}
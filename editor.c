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

char *c_keywords[4][20] = {
    {"for", "do", "while", "if", "else", "switch", "sizeof", "#include", 0},
    {"return", "goto", "case", "default", "break", "continue", 0},
    {"int", "bool", "char", "wchar_t", "short", "void", "struct", "enum", 0},
    {"unsigned", "signed", "const", "static", 0},
};

char *text, *filename = 0;
int cx = 0, cy = 0;
int yscroll = 0, xscroll = 0;
enum { M_INSERT, M_CTRLK };
enum { FTYPE_TXT, FTYPE_C };
int mode = M_INSERT;
int filetype = FTYPE_TXT;

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
    /* insert character */
    int p = getCursorPos();
    int l = strlen(text);
    for(int i = l; i > p; i--)
        text[i] = text[i-1];
    text[p] = c;
    cx++;
    
    /* handle newline */
    if(c == '\n') {
        cx = 0;
        cy++;
        l++;
        p++;
        /* calculate indentation */
        int indent = 0;
        int i;
        for(i = p-2; i > 0 && text[i] != '\n'; i--);
        for(i++; text[i] == ' '; i++)
            indent++;
        /* apply indentation */
        for(int i = l+indent; i >= p+indent; i--)
            text[i] = text[i-indent];
        for(int i = p; i < p+indent; i++)
            text[i] = ' ';
        cx = indent;
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

int streq(const char *s1, const char *s2) {
    for(int i = 0; s1[i] || s2[i]; i++)
        if(s1[i] != s2[i])
            return 0;
    return 1;
}

void loadFile(const char *filename) {
    /* Load the file */
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
    
    /* Fix tabs */
    for(int i = 0; text[i]; i++)
        if(text[i] == '\t') {
            setCursorPos(i);
            deleteChar();
            insertTab();
        }
    
    /* Determine filetype */
    const char *ext;
    for(ext = filename; ext[0] && ext[0] != '.'; ext++);
    if(streq(ext, ".c"))
        filetype = FTYPE_C;
    else
        filetype = FTYPE_TXT;
}

void saveFile(const char *filename) {
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "%s", text);
    fclose(fp);
}

void findNext(char c) {
    int p = getCursorPos();
    for(p++; text[p] && text[p] != c; p++);
    setCursorPos(p);
}

void findPrev(char c) {
    int p = getCursorPos();
    for(p--; text[p] && text[p-1] != c; p--);
    setCursorPos(p);
}

void deleteLine() {
    cx = 0;
    int w;
    int p = getCursorPos();
    for(w = p; text[w] && text[w] != '\n'; w++);
    w -= p;
    for(int i = p; text[i]; i++)
        text[i] = text[i+w];
    deleteChar();
}

void initCurses() {
    initscr();
    keypad(stdscr, 1);
    
    /* init colours */
    start_color();
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
}

void endCurses() {
    keypad(stdscr, 0);
    endwin();
}

int stringInArr(char *str, char **arr) {
    for(int i = 0; arr[i]; i++)
        if(streq(str, arr[i]))
            return 1;
    return 0;
}

void printWord(char *word, int x, int y) {
    int colour = 0;
    int i;
    switch(filetype) {
    case FTYPE_TXT:
        colour=0;
        break;
    case FTYPE_C:
        for(i = 0; i < 4; i++)
            if(stringInArr(word, c_keywords[i]))
                colour = i+1;
        break;
    }
    attron(COLOR_PAIR(colour));
    if(colour != 0)
        attron(A_BOLD);
    mvaddstr(y, x, word);
    attrset(A_NORMAL);
}

void draw() {
    int w, h;
    getmaxyx(stdscr, h, w);
    
    int yo = 1;
    int xo = 0;
    
    clear();

    /* text */    
    int x = 0, y = 0;    
    int ws = 0;
    int comment=0, quote=0;
    for(int i = 0; text[i] && y+yo-yscroll < h; i++) {
        char c = text[i];

        /* handle C-style comments */
        if(c == '/' && comment == 1 && !quote)
            if(text[i-1] == '*')
                comment = -1;
        if(c == '/' && text[i+1] == '*' && !comment && !quote)
            comment = 1;
        
        /* handle C++ style comments */
        if(text[i-1] == '\n' && comment == 2 && !quote)
            comment = 0;
        if(c == '/' && !comment && !quote)
            if(text[i+1] == '/')
                comment = 2;
        
        /* handle single quotes */
        if(text[i-1] == '\'' && quote == 1 && !comment)
            if(text[i-2] != '\\' || text[i-3] == '\\')
                quote = 0;
        if(c == '\'' && !quote && !comment)
            quote = 1;
        
        /* handle double quotes */
        if(text[i-1] == '"' && quote == 2 && !comment)
            if(text[i-2] != '\\' || text[i-3] == '\\')
                quote = 0;
        if(c == '"' && !quote && !comment)
            quote = 2;
            
        /* check whether character might be part of a keyword */
        if((c>0x40&&c<0x5b)||(c>0x60&&c<0x7b)||c=='#'||c=='_') {
            ws++;
            continue;
        }
        
        if(comment) {
            attrset(COLOR_PAIR(5));
            if(comment == -1)
                comment = 0;
        }
        
        if(quote)
            attron(COLOR_PAIR(2));
        
        if(ws > 0) {
            char *word = calloc(1, ws+1);
            for(int j = 0; j < ws; j++)
                word[j] = text[i-ws+j];
            word[ws] = 0;
            if(comment || quote)
                mvaddstr(y+yo-yscroll, x+xo-xscroll, word);
            else
                printWord(word, x+xo-xscroll, y+yo-yscroll);
            x += strlen(word);
            free(word);
        }
        
        ws = 0;
        mvaddch(y+yo-yscroll, x+xo-xscroll, c);
        x++;
        if(c == '\n') {
            x = 0;
            y++;
        }
        
        attrset(A_NORMAL);
    }
    
    /* top bar */
    attron(A_STANDOUT);
    for(int x = 0; x < w; x++)
        mvaddch(0, x, ' ');
    char coords[100];
    sprintf(coords, "%d,%d", cy+1, cx+1);
    mvaddstr(0, w-strlen(coords)-1, coords);
    const char *editingMsg = "Editing";
    char *nofileMsg = "[UNNAMED_FILE]";
    move(0, 1);
    char *name = filename;
    if(!name)
        name = nofileMsg;
    printw("%s: %s ", editingMsg, name);
    switch(filetype) {
    case FTYPE_TXT:
        printw("(txt) ");
        break;
    case FTYPE_C:
        printw("(c) ");
        break;
    }
    switch(mode) {
    case M_INSERT:
        printw("I");
        break;
    case M_CTRLK:
        printw("^K");
        break;
    }
    attrset(A_NORMAL);
    
    move(cy+yo-yscroll, cx+xo-xscroll);
    refresh();
}

void input() {
    wchar_t c = getch();
    if(mode == M_CTRLK) {
        switch(c) {
        case 's': // save
            saveFile(filename);
            break;
        case 'q': // quit
            free(text);
            endCurses();
            exit(0);
            break;
        case 'x': // save and quit
            saveFile(filename);
            free(text);
            endCurses();
            exit(0);
            break;
        default:
            break;
        }
        mode = M_INSERT;
        return;
    }
    
    int p, w, h;
    getmaxyx(stdscr, h, w);
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
        if(text[p] != '\n') {
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
    case 0x09: // TAB
        insertTab();
        break;
    case KEY_END:
        p = getCursorPos();
        if(text[p] != '\n')
            findNext('\n');
        break;
    case KEY_HOME:
        p = getCursorPos();
        if(text[p-1] != '\n')
            findPrev('\n');
        break;
    case 0x0152: // PageDown
        cy += h;
        getCursorPos();
        break;
    case 0x0153: // PageUp
        cy -= h;
        if(cy < 0)
            cy = 0;
        getCursorPos();
        break;
    case 0x0b: // CTRL+K
        mode = M_CTRLK;
        break;
    case 0x19: // CTRL+Y
        deleteLine();
        break;
    default: // character to be typed
        if((c < 0x20 && c != '\n') || c == 0x7f || c >= 0xff)
            break;
        insertChar((char)c);
        break;
    }
    
    if(strlen(text)+20 > sizeof(*text)) {
        text = realloc(text, strlen(text)+40);
        for(int i = strlen(text); i < strlen(text)+40; i++)
            text[i] = 0;
    }
    
    
    if(cx-xscroll > w-6)
        xscroll = -w+6+cx;
    if(cx-xscroll < 10)
        xscroll = -10+cx;
    if(xscroll < 0)
        xscroll = 0;
    
    if(cy-yscroll > h-3)
        yscroll = -h+3+cy;
    if(cy-yscroll < 3)
        yscroll = -3+cy;
    if(yscroll < 0)
        yscroll = 0;
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

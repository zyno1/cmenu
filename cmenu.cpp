/*
MIT License

Copyright (c) 2020 Olivier Zeyen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include<iostream>
#include<vector>
#include<string>
#include<cstring>
#include<cctype>
#include<algorithm>
#include<fstream>

#include<ncurses.h>

typedef struct Settings {
    bool caseSensitive;
    std::string prompt;
} Settings;

std::string toLowerCase(std::string const& in) {
    std::string res;
    res.reserve(in.length());

    for(auto& i : in) {
        res.push_back(std::tolower(i));
    }

    return res;
}

void filter(std::vector<std::string> const& l, std::string const& in, Settings const& set, std::vector<std::string> & res) {
    res.clear();

    auto tf = in;
    if(!set.caseSensitive) {
        tf = toLowerCase(in);
    }

    for(int i = 0; i < l.size(); i++) {
        std::string tmp = l[i];
        if(!set.caseSensitive) {
            tmp = toLowerCase(l[i]);
        }

        if(tmp.find(tf) == 0) {
            res.push_back(l[i]);
        }
    }
    for(int i = 0; i < l.size(); i++) {
        std::string tmp = l[i];
        if(!set.caseSensitive) {
            tmp = toLowerCase(l[i]);
        }

        auto p = tmp.find(tf);
        if(p != 0 && p != std::string::npos) {
            res.push_back(l[i]);
        }
    }
}

void read_list(std::vector<std::string> & list, std::istream & in) {
    std::string l;
    while(std::getline(in, l)) {
        list.push_back(l);
    }
}

void clear_screen() {
    for(int j = 0; j < LINES; j++) {
        for(int i = 0; i < COLS; i++) {
            move(j, i);
            addch(' ');
        }
    }
}

void print_list(std::vector<std::string> const& list, std::string const& line, int selected, Settings const& set) {
    int min = selected / (LINES - 1) * (LINES - 1);
    for(int i = min; i < list.size() && i - min < LINES - 1; i++) {
        move(i - min, 0);

        if(i == selected) {
            attron(A_REVERSE);
        }
        if(list[i] == "") {
            printw(" ");
        }
        else {
            if(list[i].length() > COLS) {
                printw("%s", list[i].substr(0, COLS).c_str());
            }
            else {
                printw("%s", list[i].c_str());
            }
        }
        attroff(A_REVERSE);
    }

    move(LINES - 1, 0);
    attron(A_REVERSE);
    printw("%s", set.prompt.c_str());
    attroff(A_REVERSE);
    printw("%s", line.c_str());
}

void rm_last_word(std::string & l) {
    int i;
    for(i = l.length() - 1; i >= 0 && l[i] == ' '; i--) {
        l.pop_back();
    }

    for(i = l.length() - 1; i >= 0 && l[i] != ' '; i--) {
        l.pop_back();
    }
}

std::string select(std::vector<std::string> const& l, Settings const& set) {
    std::vector<std::string> filtered;
    filtered.reserve(l.size());
    std::string line = "";
    int ch;
    int selected = 0;

    filter(l, line, set, filtered);

    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    clear_screen();
    print_list(filtered, line, selected, set);
    refresh();

    ch = getch();
    while(ch != '\n') {
        if(ch == KEY_BACKSPACE && line.length() > 0) {
            line.pop_back();
            filter(l, line, set, filtered);
            selected = std::min(selected, (int)filtered.size() - 1);
        }
        else if(ch == KEY_DOWN || ch == KEY_RIGHT) {
            selected = std::min(selected + 1, (int)filtered.size() - 1);
        }
        else if(ch == KEY_UP || ch == KEY_LEFT) {
            selected = std::max(selected - 1, 0);
        }
        else if(ch == KEY_NPAGE) {
            selected = std::min(selected + LINES - 1, (int)filtered.size() - 1);
        }
        else if(ch == KEY_PPAGE) {
            selected = std::max(selected - LINES + 1, 0);
        }
        //27 -> Escape key
        //24 -> Ctrl + X
        //-> Cancel
        else if(ch == 24 || ch == 27) {
            selected = -1;
            break;
        }
        //23 -> Ctrl + W
        else if(ch == 23) {
            rm_last_word(line);
            filter(l, line, set, filtered);
            selected = std::min(selected, (int)filtered.size() - 1);
        }
        else if(ch == '\t') {
            line = filtered[selected];
            selected = 0;
            filter(l, line, set, filtered);
            selected = std::min(selected, (int)filtered.size() - 1);
        }
        else if(isprint(ch)) {
            line += ch;
            filter(l, line, set, filtered);
            selected = std::min(selected, (int)filtered.size() - 1);
        }

        clear_screen();
        print_list(filtered, line, selected, set);
        refresh();

        ch = getch();
    }

    endwin();

    if(selected < filtered.size() && selected >= 0) {
        return filtered[selected];
    }
    if(filtered.size() == 0) {
        return line;
    }
    return "";
}

int main(int argc, char** argv) {
    std::vector<std::string> l;
    std::string in = "";
    std::string out = "";

    Settings set;
    set.caseSensitive = true;
    set.prompt = "choice: ";

    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "--in") == 0) {
            in = std::string(argv[++i]);
        }
        else if(strcmp(argv[i], "--out") == 0) {
            out = std::string(argv[++i]);
        }
        else if(strcmp(argv[i], "--ci") == 0) {
            set.caseSensitive = false;
        }
        else if(strcmp(argv[i], "--prompt") == 0) {
            set.prompt = std::string(argv[++i]);
        }
    }

    if(in == "" || out == "") {
        std::cerr << "error --in and --out mandatory\n";
        return 1;
    }

    std::ifstream fin(in);
    read_list(l, fin);
    fin.close();

    auto res = select(l, set);

    std::ofstream fout(out);
    fout << res;
    fout.close();

    return 0;
}


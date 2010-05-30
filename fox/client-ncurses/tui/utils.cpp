/*
    Copyright (C) 2006 - 2009 Rick Helmus (rhelmus_AT_gmail.com)

    This file is part of Nixstaller.
    
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version. 
    
    This program is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
    PARTICULAR PURPOSE. See the GNU General Public License for more details. 
    
    You should have received a copy of the GNU General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
    St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <assert.h>

#include "utf8.h"

#include "tui.h"
#include "widget.h"
#include "group.h"

namespace {

void Check(int ret, const char *function)
{
    if (ret == ERR)
    {
        std::string msg("ncurses function \'");
        msg += function;
        msg += "returned an error.";
   
        throw NNCurses::CTUIException(msg.c_str());
    }   
}

}


namespace NNCurses {

// -------------------------------------
// Utils
// -------------------------------------

int GetWX(WINDOW *w)
{
    int x, y;
    getbegyx(w, y, x);
    return x;
}

int GetWY(WINDOW *w)
{
    int x, y;
    getbegyx(w, y, x);
    return y;
}

int GetWWidth(WINDOW *w)
{
    int x, y;
    getmaxyx(w, y, x);
    return x-GetWX(w);
}

int GetWHeight(WINDOW *w)
{
    int x, y;
    getmaxyx(w, y, x);
    return y-GetWY(w);
}

bool IsParent(CWidget *parent, CWidget *child)
{
    CWidget *w = child->GetParentWidget();
    while (w)
    {
        if (w == parent)
            return true;
        w = w->GetParentWidget();
    }
    
    return false;
}

bool IsChild(CWidget *child, CWidget *parent)
{
    return IsParent(parent, child);
}

bool IsDirectChild(CWidget *child, CWidget *parent)
{
    return (child->GetParentWidget() == parent);
}

void AddCh(CWidget *widget, int x, int y, chtype ch)
{
    /*Check*/(mvwaddch(widget->GetWin(), y, x, ch), "mvwaddch");
}

void AddStr(CWidget *widget, int x, int y, const char *str)
{
    /*Check*/(mvwaddstr(widget->GetWin(), y, x, str), "mvwaddstr");
}

void SetAttr(CWidget *widget, chtype attr, bool e)
{
    if (e)
        Check(wattron(widget->GetWin(), attr), "wattron");
    else
        Check(wattroff(widget->GetWin(), attr), "wattroff");
}

void MoveWin(CWidget *widget, int x, int y)
{
    Check(mvwin(widget->GetWin(), y, x), "mvwin");
}

void MoveDerWin(CWidget *widget, int x, int y)
{
    Check(mvderwin(widget->GetWin(), y, x), "mvderwin");
}

void Border(CWidget *widget)
{
    Check(wborder(widget->GetWin(), 0, 0, 0, 0, 0, 0, 0, 0), "wborder");
}

void EndWin()
{
    /*Check*/(endwin(), "endwin");
}

void NoEcho()
{
    Check(noecho(), "noecho");
}

void CBreak()
{
    Check(cbreak(), "cbreak");
}

void KeyPad(WINDOW *win, bool on)
{
    Check(keypad(win, on), "keypad");
}

void Meta(WINDOW *win, bool on)
{
    Check(meta(win, on), "meta");
}

void StartColor()
{
    Check(start_color(), "start_color");
}

void InitPair(short pair, short f, short b)
{
    Check(init_pair(pair, f, b), "init_pair");
}

void Move(int x, int y)
{
    Check(move(y, x), "move");
}

void DelWin(WINDOW *win)
{
    Check(delwin(win), "delwin");
}

void WindowResize(CWidget *widget, int w, int h)
{
    Check(wresize(widget->GetWin(), h, w), "wresize");
}

void WindowErase(CWidget *widget)
{
    Check(werase(widget->GetWin()), "werase");
}

void Refresh(CWidget *widget)
{
    Check(wrefresh(widget->GetWin()), "wrefresh");
}

void WNOUTRefresh(WINDOW *win)
{
    Check(wnoutrefresh(win), "wnoutrefresh");
}

void DoUpdate()
{
    Check(doupdate(), "doupdate");
}

void HLine(CWidget *widget, int x, int y, chtype ch, int count)
{
    Check(mvwhline(widget->GetWin(), y, x, ch, count), "mvwhline");
}

TSTLStrSize MBWidth(std::string str)
{
    if (str.empty())
        return 0;
    
    TSTLStrSize ret = 0;
    std::wstring utf16;
    std::string::iterator end = utf8::find_invalid(str.begin(), str.end());
    assert(end == str.end());
    
    // HACK: Just append length of invalid part
    if (end != str.end())
        ret = std::distance(end, str.end());
    
    utf8::utf8to16(str.begin(), end, std::back_inserter(utf16));
    
    // As wcswidth returns -1 when it finds a non printable char, we just use
    // wcwidth and assume a 0 width for those chars.
    TSTLStrSize n = 0, length = utf16.length();
    for (; n<length; n++)
    {
        int w = wcwidth(utf16[n]);
        if (w > 0)
            ret += static_cast<TSTLStrSize>(w);
    }
    
    return ret;
}

TSTLStrSize GetMBLenFromW(const std::string &str, size_t width)
{
    if (str.empty())
        return 0;
    
    utf8::iterator<std::string::const_iterator> it(str.begin(), str.begin(), str.end());
    for (; it.base() != str.end(); it++)
    {
        TSTLStrSize w = MBWidth(std::string(str.begin(), it.base()));
        if (w == width)
            break;
        else if (w > width) // This may happen with multi-column chars
        {
            if (it.base() != str.begin())
                it--;
            break;
        }
    }
    
    return std::distance(str.begin(), it.base());
}

void ConvertTabs(std::string &text)
{
    TSTLStrSize length = text.length(), start = 0, startline = 0;
    while (start < length)
    {
        start = text.find_first_of("\t\n", start);
        
        if (start != std::string::npos)
        {
            if (text[start] == '\n')
                startline = 0;
            else
            {
                int rest = (startline) ? (8 % startline) : 0;
                text.replace(start, 1, 8-rest, ' ');
            }
        }
        else
            break;
        
        start++;
    }
}


}

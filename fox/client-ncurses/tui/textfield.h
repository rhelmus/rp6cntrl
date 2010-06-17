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

#ifndef TEXTWINDOW
#define TEXTWINDOW

#include "basescroll.h"

namespace NNCurses {

class CTextWidget;

class CTextField: public CBaseScroll
{
    CTextWidget *m_pTextWidget;
    bool m_bFollow, m_bScrollToBottom;

protected:
    virtual void CoreDraw(void);
    virtual bool CoreHandleKey(wchar_t key);
    virtual int CoreRequestWidth(void);
    virtual int CoreRequestHeight(void);
    virtual void CoreDrawLayout(void);
    virtual void CoreGetButtonDescs(TButtonDescList &list);
    virtual void CoreScroll(int vscroll, int hscroll);
    virtual TScrollRange CoreGetRange(void);
    virtual TScrollRange CoreGetScrollRegion(void);
    
public:
    CTextField(int maxw, int maxh, bool w);
    
    void AddText(const std::string &t);
    void SetText(const std::string &t) { ClearText(); AddText(t); }
    void SetMaxChars(TSTLStrSize m, TSTLStrSize c);
    void ClearText(void);
    void LoadFile(const char *f);
    void SetFollow(bool f);
};

}


#endif

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

#ifndef BASESCROLL
#define BASESCROLL

#include "group.h"

namespace NNCurses {

class CScrollbar;

class CBaseScroll: public CGroup
{
protected:
    typedef std::pair<int, int> TScrollRange;
    
private:
    CScrollbar *m_pVScrollbar, *m_pHScrollbar;
    TScrollRange m_CurRange;
    
    void DoScroll(void);
    void SyncBars(void);
    
protected:
    virtual void UpdateSize(void) { UpdateLayout(); CGroup::UpdateSize(); }
    virtual void CoreDraw(void);
    virtual void CoreDrawLayout(void);
    virtual void CoreScroll(int, int) {  }
    virtual TScrollRange CoreGetRange(void) = 0;
    virtual TScrollRange CoreGetScrollRegion(void) = 0;

    void VScroll(int n, bool relative);
    void HScroll(int n, bool relative);

    CBaseScroll(void);
};


}


#endif

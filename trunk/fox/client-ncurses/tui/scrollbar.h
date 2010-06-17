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

#ifndef SCROLLBAR
#define SCROLLBAR

#include "tui.h"
#include "widget.h"

namespace NNCurses {

class CScrollbar: public CWidget
{
public:
    enum EType { HORIZONTAL, VERTICAL };
    
private:
    EType m_eType;
    float m_fMin, m_fMax, m_fCurrent;
    
    void SyncColors(void);
    
protected:
    virtual void CoreDraw(void);
    virtual void DoDraw(void);
    
public:
    CScrollbar(EType t) : m_eType(t), m_fMin(0.0f), m_fMax(0.0f), m_fCurrent(0.0f) { }
    
    void SetRange(int min, int max);
    void SetCurrent(int val);
    void Scroll(int n) { SetCurrent(m_fCurrent + static_cast<float>(n)); }
    int Value(void) const { return static_cast<int>(m_fCurrent); }
};


}


#endif

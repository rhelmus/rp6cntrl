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

#include "tui.h"
#include "basescroll.h"
#include "scrollbar.h"

namespace NNCurses {

// -------------------------------------
// Base Scroll Class
// -------------------------------------

CBaseScroll::CBaseScroll()
{
    SetBox(true);
    AddWidget(m_pVScrollbar = new CScrollbar(CScrollbar::VERTICAL));
    AddWidget(m_pHScrollbar = new CScrollbar(CScrollbar::HORIZONTAL));
}

void CBaseScroll::DoScroll()
{
    int v = (m_pVScrollbar->Enabled()) ? m_pVScrollbar->Value() : 0;
    int h = (m_pHScrollbar->Enabled()) ? m_pHScrollbar->Value() : 0;
    
    CoreScroll(v, h);
    RequestQueuedDraw();
}

void CBaseScroll::CoreDraw()
{
    CGroup::CoreDraw();
    // Do this after childs have been drawn: scrollbars may be dependent
    // on child widgets, which can change after a redraw
    SyncBars();
}

void CBaseScroll::SyncBars()
{
    TScrollRange range = CoreGetRange();
    TScrollRange region = CoreGetScrollRegion();
    
    m_pVScrollbar->Enable(range.first > region.first);
    m_pHScrollbar->Enable(range.second > region.second);

    range.first -= region.first;
    range.second -= region.second;
    
    if (range.first < 0)
        range.first = 0;
    
    if (range.second < 0)
        range.second = 0;
    
    if (range != m_CurRange)
    {
        m_pVScrollbar->SetRange(0, range.first);
        m_pHScrollbar->SetRange(0, range.second);
        m_CurRange = range;
    }
}

void CBaseScroll::CoreDrawLayout()
{
    SetChildSize(m_pVScrollbar, Width()-1, 1, 1, Height()-2);
    SetChildSize(m_pHScrollbar, 1, Height()-1, Width()-2, 1);
}

void CBaseScroll::VScroll(int n, bool relative)
{
    if (!m_pVScrollbar->Enabled())
        return;
    
    if (relative)
        m_pVScrollbar->Scroll(n);
    else
        m_pVScrollbar->SetCurrent(n);
    
    DoScroll();
}

void CBaseScroll::HScroll(int n, bool relative)
{
    if (!m_pHScrollbar->Enabled())
        return;

    if (relative)
        m_pHScrollbar->Scroll(n);
    else
        m_pHScrollbar->SetCurrent(n);

    DoScroll();
}

}

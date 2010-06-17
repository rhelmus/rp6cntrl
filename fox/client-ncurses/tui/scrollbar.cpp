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

#include <math.h>

#include "tui.h"
#include "group.h"
#include "scrollbar.h"

namespace NNCurses {

// -------------------------------------
// Scrollbar Class
// -------------------------------------

void CScrollbar::SyncColors()
{
    TColorPair colors;
    CWidget *p = GetParentWidget();

    if (p->Focused())
        colors = GetParentWidget()->GetFColors();
    else
        colors = GetParentWidget()->GetDFColors();
    
    SetDFColors(colors.second, colors.first); // Reverse colors
}
    
void CScrollbar::CoreDraw()
{
    SyncColors();
    InitDraw();
    DoDraw();
    RefreshWidget();
}

void CScrollbar::DoDraw()
{
    float fac = 0.0f;
    float posx, posy;
    
    if ((m_fMax - m_fMin) > 0.0f)
        fac = m_fCurrent / (m_fMax-m_fMin);
    
    if (m_eType == VERTICAL)
    {
        posx = 0.0f;
        posy = (float)(Height()-1) * fac;
    }
    else
    {
        posx = (float)(Width()-1) * fac;
        posy = 0.0f;
    }
    
    if (posx < 0.0f)
        posx = 0.0f;
    if (posy < 0.0f)
        posy = 0.0f;
    
    AddCh(this, SafeConvert<int>(posx), SafeConvert<int>(posy), ACS_CKBOARD);
}

void CScrollbar::SetRange(int min, int max)
{
    m_fMin = static_cast<float>(min);
    m_fMax = static_cast<float>(max);
    
    if (m_fCurrent < m_fMin)
        m_fCurrent = m_fMin;
    else if (m_fCurrent > m_fMax)
        m_fCurrent = m_fMax;
}

void CScrollbar::SetCurrent(int n)
{
    m_fCurrent = static_cast<float>(n);
    
    if (m_fCurrent < m_fMin)
        m_fCurrent = m_fMin;
    else if (m_fCurrent > m_fMax)
        m_fCurrent = m_fMax;
    
    RequestQueuedDraw();
}

}

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
#include "label.h"

namespace NNCurses {

// -------------------------------------
// Label Widget Class
// -------------------------------------

void CLabel::CoreDraw()
{
    if (!GetQueuedText().empty())
        UpdateText(RequestWidth());
    
    DrawWidget();
}

void CLabel::DoDraw()
{
    const TLinesList &lines = GetLineList();
    
    int x = 0, y = (Center()) ? ((Height() - SafeConvert<int>(lines.size())) / 2) : 0;
    
    if (y < 0)
        y = 0;
    
    for (TLinesList::const_iterator it=lines.begin(); it!=lines.end(); it++, y++)
    {
        if (y == Height())
            break;

        if (Center())
            x = ((Width() - MBWidth(*it)) / 2);

        AddStr(this, x, y, it->c_str());
    }
}


}

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

#include <fstream>
#include "tui.h"
#include "textfield.h"
#include "textwidget.h"
#include "scrollbar.h"

namespace NNCurses {

// -------------------------------------
// Text Field Class
// -------------------------------------

CTextField::CTextField(int maxw, int maxh, bool w) : m_bFollow(false), m_bScrollToBottom(false)
{
    m_pTextWidget = new CTextWidget(w);
    
    if (maxw)
    {
        m_pTextWidget->SetMinWidth(maxw);
        m_pTextWidget->SetMaxReqWidth(maxw);
    }
    
    if (maxh)
    {
        m_pTextWidget->SetMinHeight(maxh);
        m_pTextWidget->SetMaxReqHeight(maxh);
    }
    
    AddWidget(m_pTextWidget);
}

void CTextField::CoreDraw()
{
    CBaseScroll::CoreDraw();
    
    if (m_bScrollToBottom)
    {
        m_bScrollToBottom = false;
        VScroll(m_pTextWidget->GetHRange()-m_pTextWidget->Height(), false);
        HScroll(m_pTextWidget->GetWRange()-m_pTextWidget->Width(), false);
    }
}

bool CTextField::CoreHandleKey(wchar_t key)
{
    if (CBaseScroll::CoreHandleKey(key))
        return true;
    
    switch (key)
    {
        case KEY_UP:
            VScroll(-1, true);
            return true;
        case KEY_DOWN:
            VScroll(1, true);
            return true;
        case KEY_LEFT:
            HScroll(-1, true);
            return true;
        case KEY_RIGHT:
            HScroll(1, true);
            return true;
        case KEY_PPAGE:
            VScroll(-m_pTextWidget->Height(), true);
            return true;
        case KEY_NPAGE:
            VScroll(m_pTextWidget->Height(), true);
            return true;
        case KEY_HOME:
            HScroll(0, false);
            return true;
        case KEY_END:
            HScroll(m_pTextWidget->GetWRange()-m_pTextWidget->Width(), false);
            return true;
        default:
            if (IsEnter(key))
            {
                PushEvent(EVENT_CALLBACK);
                return true;
            }
            break;
    }
    
    return false;
}

int CTextField::CoreRequestWidth()
{
    return m_pTextWidget->RequestWidth() + 2; // +2 For surrounding box
}

int CTextField::CoreRequestHeight()
{
    return m_pTextWidget->RequestHeight() + 2; // +2 For surrounding box
}

void CTextField::CoreDrawLayout()
{
    SetChildSize(m_pTextWidget, 1, 1, Width()-2, Height()-2);
    CBaseScroll::CoreDrawLayout();
}

void CTextField::CoreGetButtonDescs(TButtonDescList &list)
{
    list.push_back(TButtonDescPair("ARROWS/PGUP/PGDOWN/HOME/END", "Navigate"));
    CBaseScroll::CoreGetButtonDescs(list);
}

void CTextField::CoreScroll(int vscroll, int hscroll)
{
    m_pTextWidget->SetOffset(hscroll, vscroll);
}

CTextField::TScrollRange CTextField::CoreGetRange()
{
    return TScrollRange(SafeConvert<int>(m_pTextWidget->GetHRange()), SafeConvert<int>(m_pTextWidget->GetWRange()));
}

CTextField::TScrollRange CTextField::CoreGetScrollRegion()
{
    return TScrollRange(m_pTextWidget->Height(), m_pTextWidget->Width());
}

void CTextField::AddText(const std::string &t)
{
    // This will trigger the following in order:
    // 1) Text will be queued
    // 2) After a redraw the text will be added
    // 3) if m_bFollow is true the textwidget will be scrolled down
    m_pTextWidget->AddText(t);
    m_bScrollToBottom = m_bFollow;
    RequestQueuedDraw();
}

void CTextField::SetMaxChars(TSTLStrSize m, TSTLStrSize c)
{
    m_pTextWidget->SetMaxChars(m, c);
    RequestQueuedDraw();
}

void CTextField::ClearText()
{
    m_pTextWidget->Clear();
    RequestQueuedDraw();
}

void CTextField::LoadFile(const char *f)
{
    std::ifstream file(f);
    std::string buf;
    char c;
    
    while (file && file.get(c))
        buf += c;
    
    AddText(buf);
}

void CTextField::SetFollow(bool f)
{
    m_bFollow = f;
    if (f)
    {
        m_bScrollToBottom = true;
        RequestQueuedDraw();
    }
}

}

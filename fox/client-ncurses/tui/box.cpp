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
#include "tui.h"
#include "box.h"

namespace NNCurses {

// -------------------------------------
// Widget Box Class
// -------------------------------------

int CBox::GetWidgetW(CWidget *w)
{
    int ret = 0;
    
    if (m_bEqual)
    {
        // UNDONE: Cache?
        const TChildList &childs = GetChildList();
        
        for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
        {
            if (!IsValidWidget(*it))
                continue;
            
            ret = std::max(ret, (*it)->RequestWidth());
        }
    }
    else
        ret = w->RequestWidth();
    
    return ret;
}

int CBox::GetWidgetH(CWidget *w)
{
    int ret = 0;
    
    if (m_bEqual)
    {
        // UNDONE: Cache?
        const TChildList &childs = GetChildList();
        
        for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
        {
            if (!IsValidWidget(*it))
                continue;
            
            ret = std::max(ret, (*it)->RequestHeight());
        }
    }
    else
        ret = w->RequestHeight();
    
    return ret;
}

int CBox::GetTotalWidgetW(CWidget *w)
{
    const SBoxEntry &entry = m_BoxEntries[w];
    int ret = 0;

    if (m_eDirection == HORIZONTAL)
    {
        ret += GetWidgetW(w) + (2 * entry.hpadding);
        if (w != GetChildList().back())
            ret += m_iSpacing;
    }
    else
        ret = w->RequestWidth() + (2 * entry.hpadding);
    
    return ret;
}

int CBox::GetTotalWidgetH(CWidget *w)
{
    const SBoxEntry &entry = m_BoxEntries[w];
    int ret = 0;

    if (m_eDirection == VERTICAL)
    {
        ret = (GetWidgetH(w) + (2 * entry.vpadding));
            
        if (w != GetChildList().back())
            ret += m_iSpacing;
    }
    else
        ret = w->RequestHeight() + (2 * entry.vpadding);
    
    return ret;
}

int CBox::RequestedWidgetsW()
{
    const TChildList &childs = GetChildList();
    int ret = 0;
    
    for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
    {
        if (!IsValidWidget(*it))
            continue;
        
        if (m_eDirection == HORIZONTAL)
            ret += GetTotalWidgetW(*it);
        else
            ret = std::max(ret, GetTotalWidgetW(*it));
    }
    
    return ret;
}

int CBox::RequestedWidgetsH()
{
    const TChildList &childs = GetChildList();
    int ret = 0;
    
    for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
    {
        if (!IsValidWidget(*it))
            continue;

        if (m_eDirection == VERTICAL)
            ret += GetTotalWidgetH(*it);
        else
            ret = std::max(ret, GetTotalWidgetH(*it));
    }
    
    return ret;
}

CBox::TChildList::size_type CBox::ExpandedWidgets()
{
    const TChildList &childs = GetChildList();
    TChildList::size_type ret = 0;
    
    for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
    {
        if (!IsValidWidget(*it))
            continue;
        
        if (!m_bEqual && !m_BoxEntries[*it].expand)
            continue;
        
        ret++;
    }
    
    return ret;
}

bool CBox::IsValidWidget(CWidget *w)
{
    return w->Enabled();
}

void CBox::CoreDrawLayout()
{
    if (Empty())
        return;

    const TChildList &childs = GetChildList();
    const TChildList::size_type size = ExpandedWidgets();
    const int diffw = FieldWidth() - RequestedWidgetsW();
    const int diffh = FieldHeight() - RequestedWidgetsH();
    const int extraw = (size) ? diffw / size : 0;
    const int extrah = (size) ? diffh / size : 0;
    int remainingw = (size) ? diffw % size : 0;
    int remainingh = (size) ? diffh % size : 0;
    int begx = FieldX(), begy = FieldY(); // Coords for widgets that were packed at start
    int endx = (FieldX()+FieldWidth())-1, endy = (FieldY()+FieldHeight())-1; // Coords for widgets that were packed at end

    for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
    {
        if (!IsValidWidget(*it))
            continue;
        
        const SBoxEntry &entry = m_BoxEntries[*it];
        int basex = entry.hpadding, basey = entry.vpadding;
        int widgetw = 0, widgeth = 0;
        int basew = extraw, baseh = extrah;
        int spacing = 0;
    
        if (*it != childs.back())
            spacing += m_iSpacing;
    
        if (remainingw)
        {
            basew++;
            remainingw--;
        }
    
        if  (remainingh)
        {
            baseh++;
            remainingh--;
        }
    
        if (m_eDirection == HORIZONTAL)
        {
            spacing += entry.hpadding;
            widgetw += GetWidgetW(*it);
            widgeth += (FieldHeight() - (basey*entry.vpadding));
        
            if (entry.expand)
            {
                if (entry.fill)
                    widgetw += basew;
                else
                {
                    spacing += (basew/2);
                    basex += (basew/2);
                }
            }
        }
        else
        {
            spacing += entry.vpadding;
            widgetw += (FieldWidth() - (basex+entry.hpadding));
            widgeth += GetWidgetH(*it);
        
            if (entry.expand)
            {
                if (entry.fill)
                    widgeth += baseh;
                else
                {
                    spacing += (baseh/2);
                    basey += (baseh/2);
                }
            }
        }
    
        assert(begx+basex >= FieldX());
        assert(begy+basey >= FieldY());
        assert(widgetw>0 && widgeth>0);
        assert(widgetw >= (*it)->RequestWidth());
        assert(widgeth >= (*it)->RequestHeight());
        
        if (entry.start)
        {
            SetChildSize(*it, begx+basex, begy+basey, widgetw, widgeth);
        
            if (m_eDirection == HORIZONTAL)
                begx += (basex + spacing + widgetw);
            else
                begy += (basey + spacing + widgeth);
        }
        else
        {
            SetChildSize(*it, endx-basex-(widgetw-1), endy-basey-(widgeth-1), widgetw, widgeth);
        
            if (m_eDirection == HORIZONTAL)
                endx -= (basex + spacing + widgetw);
            else
                endy -= (basey + spacing + widgeth);
        }
    }
}

int CBox::CoreRequestWidth()
{
    int ret = RequestedWidgetsW();
    
    if (HasBox())
        ret += 2;
    
    return std::max(ret, GetMinWidth());
}

int CBox::CoreRequestHeight()
{
    int ret = RequestedWidgetsH();
    
    if (HasBox())
        ret += 2;

    return std::max(ret, GetMinHeight());
}

bool CBox::CoreHandleEvent(CWidget *emitter, int event)
{
    if (IsChild(emitter, this))
    {
        if (event == EVENT_REQUPDATE)
        {
            UpdateLayout();
            PushEvent(event);
            
            if (!GetParentWidget())
                RequestQueuedDraw();
            
            return true;
        }
    }
    
    return CGroup::CoreHandleEvent(emitter, event);
}

void CBox::CoreRemoveWidget(CWidget *w)
{
    RequestUpdate();
}

void CBox::StartPack(CGroup *g, bool e, bool f, int vp, int hp)
{
    m_BoxEntries[g] = SBoxEntry(e, f, vp, hp, true);
    CGroup::AddWidget(g);
}

void CBox::StartPack(CWidget *w, bool e, bool f, int vp, int hp)
{
    m_BoxEntries[w] = SBoxEntry(e, f, vp, hp, true);
    CGroup::AddWidget(w);
}

void CBox::EndPack(CGroup *g, bool e, bool f, int vp, int hp)
{
    m_BoxEntries[g] = SBoxEntry(e, f, vp, hp, false);
    CGroup::AddWidget(g);
}

void CBox::EndPack(CWidget *w, bool e, bool f, int vp, int hp)
{
    m_BoxEntries[w] = SBoxEntry(e, f, vp, hp, false);
    CGroup::AddWidget(w);
}

}

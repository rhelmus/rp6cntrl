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
#include <algorithm>
#include "tui.h"
#include "widget.h"
#include "group.h"

namespace NNCurses {

// -------------------------------------
// Group Widget Class
// -------------------------------------

bool CGroup::CanFocusChilds(CWidget *w)
{
    if (!IsGroupWidget(w))
        return false;
    
    CGroup *group = GetGroupWidget(w);
    const TChildList &childs = group->GetChildList();
    
    for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
    {
        if ((*it)->Enabled() && ((*it)->CanFocus() || group->CanFocusChilds(*it)))
            return true;
    }
    
    return false;
}

void CGroup::CoreDraw(void)
{
    InitDraw();
    
    if (m_bUpdateLayout)
    {
        DrawLayout();
        m_bUpdateLayout = false;
    }

    DoDraw();
    RefreshWidget();

    DrawChilds();
}

void CGroup::CoreTouchSize()
{
    for (TChildList::iterator it=m_Childs.begin(); it!=m_Childs.end(); it++)
    {
        if (!(*it)->Enabled())
            continue;
        
        (*it)->TouchSize();
    }
}

void CGroup::UpdateSize()
{
    RequestQueuedDraw();
    for (TChildList::iterator it=m_Childs.begin(); it!=m_Childs.end(); it++)
    {
        if (!(*it)->Enabled())
            continue;
        
        (*it)->TouchSize();
    }
}

void CGroup::UpdateFocus()
{
    if (m_pFocusedWidget)
        m_pFocusedWidget->SetFocus(Focused());
    else if (Focused())
        SetNextFocWidget(false);
    
    for (TChildList::iterator it=m_Childs.begin(); it!=m_Childs.end(); it++)
    {
        if ((*it == m_pFocusedWidget) || !(*it)->Enabled())
            continue;
        
        // Let all other widgets update their colors. The group has changed it's color which may affect any childs
        (*it)->TouchColor();
        (*it)->RequestQueuedDraw();
    }
}

bool CGroup::CoreHandleKey(wchar_t key)
{
    if (m_pFocusedWidget)
        return m_pFocusedWidget->HandleKey(key);

    return false;
}

bool CGroup::CoreHandleEvent(CWidget *emitter, int event)
{
    if (event == EVENT_REQFOCUS)
    {
        FocusWidget(emitter);
        PushEvent(EVENT_REQFOCUS);
        return true;
    }

    return CWidget::CoreHandleEvent(emitter, event);
}

void CGroup::CoreDrawChilds(void)
{
    for (TChildList::iterator it=m_Childs.begin(); it!=m_Childs.end(); it++)
    {
        if ((*it)->Enabled())
            (*it)->Draw();
    }
}

void CGroup::CoreGetButtonDescs(TButtonDescList &list)
{
    if (m_pFocusedWidget)
        m_pFocusedWidget->GetButtonDescs(list);
}

void CGroup::AddWidget(CGroup *g)
{
    m_GroupMap[g] = g;
    AddWidget(static_cast<CWidget *>(g));
}

void CGroup::InitChild(CWidget *w)
{
    m_Childs.push_back(w);
    w->SetParent(this);
}

void CGroup::RemoveWidget(CWidget *w)
{
    CoreRemoveWidget(w);
    
    if (m_pFocusedWidget == w)
        SetValidWidget(w);
    
    TChildList::iterator it = std::find(m_Childs.begin(), m_Childs.end(), w);
    assert(it != m_Childs.end());
    m_Childs.erase(it);
    m_GroupMap[w] = NULL; // Is NULL already if w isn't a group
}

void CGroup::EnableWidget(CWidget *w, bool e)
{
    // Widget not initialized yet? (ignore if the group isn't initialized, since it will initialize the widget later)
    if (e && GetWin() && !w->GetWin())
        RequestUpdate();
    else if (!e && (m_pFocusedWidget == w))
        SetValidWidget(w);
}

void CGroup::Clear()
{
    while (!m_Childs.empty())
    {
        // CWidget's destructor will call RemoveWidget() which removes widget from m_Childs list
        delete m_Childs.back();
    }

    m_GroupMap.clear();
}

void CGroup::CoreFocusWidget(CWidget *w)
{
    if (w == m_pFocusedWidget)
        return;
    
    if (m_pFocusedWidget)
    {
        m_pFocusedWidget->SetFocus(false);
        m_pFocusedWidget->RequestQueuedDraw();
    }
    
    m_pFocusedWidget = w;
    
    if (w && Focused()) // If w == NULL the current focused widget is reset
    {
        w->SetFocus(true);
        w->RequestQueuedDraw();
    }
}

void CGroup::RequestUpdate()
{
    UpdateLayout();
    PushEvent(EVENT_REQUPDATE);
}

bool CGroup::SetNextFocWidget(bool cont)
{
    if (m_Childs.empty())
        return false;

    TChildList::iterator it = m_Childs.begin();
    
    if (cont && m_pFocusedWidget && Focused())
    {
        TChildList::iterator f = std::find(m_Childs.begin(), m_Childs.end(), m_pFocusedWidget);
        if (f != m_Childs.end())
        {
            it = f;
            if (!CanFocusChilds(*it))
                it++;
        }
    }
    
    for (; it!=m_Childs.end(); it++)
    {
        if (!(*it)->Enabled())
            continue;
        
        if (!(*it)->CanFocus())
        {
            if (CanFocusChilds(*it))
            {
                if (!GetGroupWidget(*it)->SetNextFocWidget(cont))
                    continue;
            }
            else
                continue;
        }

        FocusWidget(*it);
        return true;
    }
    
    return false;
}

bool CGroup::SetPrevFocWidget(bool cont)
{
    if (m_Childs.empty())
        return false;

    TChildList::reverse_iterator it = m_Childs.rbegin();
    
    if (cont && m_pFocusedWidget && Focused())
    {
        TChildList::reverse_iterator f = std::find(m_Childs.rbegin(), m_Childs.rend(), m_pFocusedWidget);
        if (f != m_Childs.rend())
        {
            it = f;
            if (!CanFocusChilds(*it))
                it++;
        }
    }
    
    for (; it!=m_Childs.rend(); it++)
    {
        if (!(*it)->Enabled())
            continue;

        if (!(*it)->CanFocus())
        {
            if (CanFocusChilds(*it))
            {
                if (!GetGroupWidget(*it)->SetPrevFocWidget(cont))
                    continue;
            }
            else
                continue;
        }

        FocusWidget(*it);
        return true;
    }
    
    return false;
}

void CGroup::SetValidWidget(CWidget *ignore)
{
    TChildList::iterator itprev, itnext;
    
    itprev = itnext = std::find(m_Childs.begin(), m_Childs.end(), ignore);
    
    assert(itprev != m_Childs.end());
    
    do
    {
        if (itprev != m_Childs.begin())
        {
            itprev--;
            
            if ((*itprev)->Enabled() && ((*itprev)->CanFocus() || CanFocusChilds(*itprev)))
            {
                FocusWidget(*itprev);
                return;
            }
        }
        
        if (*itnext != m_Childs.back())
        {
            itnext++;
            
            if (itnext != m_Childs.end())
            {
                if ((*itnext)->Enabled() && ((*itnext)->CanFocus() || CanFocusChilds(*itnext)))
                {
                    FocusWidget(*itnext);
                    return;
                }
            }
        }
    }
    while ((itprev != m_Childs.begin()) || (*itnext != m_Childs.back()));
    
    // Haven't found a focusable widget
    FocusWidget(NULL);
}

}

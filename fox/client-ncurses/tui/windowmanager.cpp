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

#include <algorithm>
#include "tui.h"
#include "windowmanager.h"

namespace NNCurses {

// -------------------------------------
// Window Manager Class
// -------------------------------------

bool CWindowManager::CoreHandleKey(wchar_t key)
{
    if (CGroup::CoreHandleKey(key))
        return true;
    
    CWidget *focwidget = GetFocusedWidget();
    if (focwidget)
    {
        CGroup *focgroup = GetGroupWidget(focwidget);
        if (focgroup)
        {
            if (IsTAB(key))
            {
                if (!focgroup->SetNextFocWidget(true))
                    focgroup->SetNextFocWidget(false);
                TUI.UpdateButtonBar();
                return true;
            }
            else if (key == CTRL('p'))
            {
                if (!focgroup->SetPrevFocWidget(true))
                    focgroup->SetPrevFocWidget(false);
                TUI.UpdateButtonBar();
                return true;
            }
        }
    }
    
    return false;
}

bool CWindowManager::CoreHandleEvent(CWidget *emitter, int event)
{
    if (CGroup::CoreHandleEvent(emitter, event))
        return true;
    
    if (event == EVENT_REQQUEUEDDRAW)
    {
        CWidget *focwidget = GetFocusedWidget();
        if (focwidget)
        {
            if ((focwidget != emitter) && !IsChild(emitter, focwidget))
                focwidget->RequestQueuedDraw();
        }
        
        return true;
    }
    else if (event == EVENT_REQUPDATE)
    {
        if (IsDirectChild(emitter, this) && !emitter->Deleted())
        {
            m_WidgetQueue.push_back(emitter);
            UpdateLayout();
            PushEvent(EVENT_REQUPDATE);
            return true;
        }
    }
    
    return false;
}

int CWindowManager::CoreRequestWidth()
{
    const TChildList &childs = GetChildList();
    int ret = GetMinWidth();
    
    for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
    {
        if ((*it)->Enabled())
            ret = std::max(ret, (*it)->RequestWidth());
    }
    
    return ret;
}

int CWindowManager::CoreRequestHeight()
{
    const TChildList &childs = GetChildList();
    int ret = GetMinHeight();
    
    for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
    {
        if ((*it)->Enabled())
            ret = std::max(ret, (*it)->RequestHeight());
    }
    
    return ret;
}

void CWindowManager::CoreGetButtonDescs(TButtonDescList &list)
{
    list.push_back(TButtonDescPair("TAB", "Next field"));
    CGroup::CoreGetButtonDescs(list);
}

void CWindowManager::CoreDrawChilds()
{
    const TChildList &childs = GetChildList();
    
    for (TChildList::const_iterator it=childs.begin(); it!=childs.end(); it++)
    {
        if (!(*it)->Enabled())
            continue;
        
        if (std::find(m_ActiveWidgets.begin(), m_ActiveWidgets.end(), *it) != m_ActiveWidgets.end())
            continue; // We draw these as last, so it looks like they are on top
        DrawChild(*it);
    }
    
    for (TChildList::const_iterator it=m_ActiveWidgets.begin(); it!=m_ActiveWidgets.end(); it++)
    {
        if (!(*it)->Enabled())
            continue;

        DrawChild(*it);
    }
}

void CWindowManager::CoreAddWidget(CWidget *w)
{
    m_WidgetQueue.push_back(w);
    InitChild(w);
    RequestUpdate();
}

void CWindowManager::CoreRemoveWidget(CWidget *w)
{
    if (w == m_pLockedWidget)
        m_pLockedWidget = NULL;
    
    TChildList::iterator it = std::find(m_ActiveWidgets.begin(), m_ActiveWidgets.end(), w);
    
    if (it != m_ActiveWidgets.end())
        m_ActiveWidgets.erase(it);
    
    std::deque<CWidget *>::iterator qit = std::find(m_WidgetQueue.begin(), m_WidgetQueue.end(), w);
    
    while (qit != m_WidgetQueue.end())
    {
        m_WidgetQueue.erase(qit);
        qit = std::find(m_WidgetQueue.begin(), m_WidgetQueue.end(), w);
    }
    
    RequestUpdate();
    TUI.UpdateButtonBar();
}

void CWindowManager::CoreFocusWidget(CWidget *w)
{
    if (m_pLockedWidget && (w != m_pLockedWidget))
        return;
    
    CWidget *focwidget = GetFocusedWidget();
    
    if (focwidget != w)
    {
        TChildList::iterator it = std::find(m_ActiveWidgets.begin(), m_ActiveWidgets.end(), w);
        
        if (it != m_ActiveWidgets.end())
            m_ActiveWidgets.erase(it);
        
        m_ActiveWidgets.push_back(w);
    }
    
    CGroup::CoreFocusWidget(w);
    TUI.UpdateButtonBar();
}

void CWindowManager::CoreDrawLayout()
{
    while (!m_WidgetQueue.empty())
    {
        CWidget *w = m_WidgetQueue.front();
        m_WidgetQueue.pop_front();
        
        if (w->Enabled())
        {
            const int width = w->RequestWidth(), height = w->RequestHeight();
            const int x = (Width() - width) / 2;
            const int y = (Height() - height) / 2;
        
            SetChildSize(w, x, y, width, height);
        }
    }
}

}


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

#ifndef WINDOWMANAGER
#define WINDOWMANAGER

#include <deque>
#include "group.h"

namespace NNCurses {

class CWindowManager: public CGroup
{
    std::deque<CWidget *> m_WidgetQueue;
    TChildList m_ActiveWidgets; // List of last active widgets
    CWidget *m_pLockedWidget;
    
protected:
    virtual bool CoreHandleKey(wchar_t key);
    virtual bool CoreHandleEvent(CWidget *emitter, int event);
    virtual int CoreRequestWidth(void);
    virtual int CoreRequestHeight(void);
    virtual void CoreGetButtonDescs(TButtonDescList &list);
    virtual void CoreDrawChilds(void);
    virtual void CoreAddWidget(CWidget *w);
    virtual void CoreRemoveWidget(CWidget *w);
    virtual void CoreFocusWidget(CWidget *w);
    virtual void CoreDrawLayout(void);
    
public:
    CWindowManager(void) : m_pLockedWidget(NULL) { }
    void LockWidget(CWidget *w) { m_pLockedWidget = w; }
};

}

#endif

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

#ifndef GROUP_H
#define GROUP_H

#include <vector>
#include <map>
#include "widget.h"

namespace NNCurses {

class CGroup: public CWidget
{
public:
    typedef std::vector<CWidget *> TChildList;
    
private:
    TChildList m_Childs;
    CWidget *m_pFocusedWidget;
    std::map<CWidget *, CGroup *> m_GroupMap; // Widgets that are also groups
    bool m_bUpdateLayout;
    
    bool IsGroupWidget(CWidget *w) { return (m_GroupMap[w] != NULL); }
    void DrawLayout(void) { CoreDrawLayout(); }

protected:
    CGroup(void) : m_pFocusedWidget(NULL), m_bUpdateLayout(true) { }

    virtual void CoreDraw(void);
    virtual void CoreTouchSize(void);
    virtual void UpdateSize(void);
    virtual void UpdateFocus(void);
    virtual bool CoreHandleKey(wchar_t key);
    virtual bool CoreHandleEvent(CWidget *emitter, int event);
    virtual void CoreAddWidget(CWidget *w) { InitChild(w); };
    virtual void CoreRemoveWidget(CWidget *w) { }
    virtual void CoreFocusWidget(CWidget *w);
    virtual void CoreDrawChilds(void);
    virtual void CoreGetButtonDescs(TButtonDescList &list);
    virtual void CoreDrawLayout(void) { }

    void FocusWidget(CWidget *w) { CoreFocusWidget(w); }
    void InitChild(CWidget *w);
    void DrawChilds(void) { CoreDrawChilds(); }
    CWidget *GetFocusedWidget(void) { return m_pFocusedWidget; }
    void SetChildSize(CWidget *widget, int x, int y, int w, int h) { widget->SetSize(x, y, w, h); }
    void DrawChild(CWidget *w) { w->Draw(); }
    void UpdateLayout(void) { m_bUpdateLayout = true; }
    void Clear(void);

public:
    virtual ~CGroup(void) { Clear(); };
    
    void AddWidget(CGroup *g);
    void AddWidget(CWidget *w) { CoreAddWidget(w); };
    void RemoveWidget(CWidget *w);
    void EnableWidget(CWidget *w, bool e);
    TChildList &GetChildList(void) { return m_Childs; }
    CGroup *GetGroupWidget(CWidget *w) { return m_GroupMap[w]; }
    bool Empty(void) { return m_Childs.empty(); }
    void RequestUpdate(void);
    bool CanFocusChilds(CWidget *w);

    bool SetNextFocWidget(bool cont); // cont : Checks widget after current focused widget
    bool SetPrevFocWidget(bool cont);
    void SetValidWidget(CWidget *ignore);
};

}

#endif

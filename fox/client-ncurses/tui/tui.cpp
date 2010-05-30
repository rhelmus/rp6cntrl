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
#include "assert.h"
#include "tui.h"
#include "widget.h"
#include "group.h"
#include "box.h"
#include "windowmanager.h"
#include "buttonbar.h"

// HACK: _XOPEN_SOURCE_EXTENDED needs to be defined for this, but setting
// that may cause other compile errors
#ifndef get_wch
extern "C" int get_wch (wint_t *);
#endif

namespace NNCurses {

CTUI TUI;

// -------------------------------------
// TUI (Text User Interface) Class
// -------------------------------------

void CTUI::UpdateButtonBar()
{
    m_pButtonBar->ClearButtons();
    
    TButtonDescList list;
    m_pWinManager->GetButtonDescs(list);
    
    for (TButtonDescList::iterator it=list.begin(); it!=list.end(); it++)
        m_pButtonBar->AddButton(it->first, it->second);
}


void CTUI::InitNCurses()
{
    EndWin();
    initscr();
    
    NoEcho();
    CBreak();
    leaveok(GetRootWin(), false);
    KeyPad(GetRootWin(), true);
    Meta(GetRootWin(), true);
    
    if (has_colors())
        StartColor();
    
    m_pMainBox = new CBox(CBox::VERTICAL, false);
    m_pMainBox->SetParent(GetRootWin());
    m_pMainBox->Init();
    m_pMainBox->SetSize(0, 0, GetWWidth(GetRootWin()), GetWHeight(GetRootWin()));
    m_pMainBox->SetFocus(true);
    
    m_pButtonBar = new CButtonBar(GetWWidth(GetRootWin()));
    
    // Focused colors are used for keys, defocused colors for descriptions
    m_pButtonBar->SetFColors(COLOR_YELLOW, COLOR_RED);
    m_pButtonBar->SetDFColors(COLOR_WHITE, COLOR_RED);
    
    m_pMainBox->EndPack(m_pButtonBar, false, false, 0, 0);
    
    m_pWinManager = new CWindowManager;
    m_pMainBox->AddWidget(m_pWinManager);
    m_pWinManager->ReqFocus();
    
    m_pMainBox->RequestQueuedDraw();
}

void CTUI::StopNCurses()
{
    if (m_pMainBox)
    {
        delete m_pMainBox;
        m_pMainBox = NULL;
        EndWin();
    }
}

bool CTUI::Run(int delay)
{
    if (m_bQuit)
        return false;
    
    timeout(delay);
    
    // Handle keys
    wint_t key;
    chtype keystat = get_wch(&key);
    
    if ((keystat != static_cast<chtype>(ERR)) && (key != WEOF)) // Input available?
    {
        if (!m_pWinManager->HandleKey(key) && IsEscape(key))
            return false;
    }
    
    Move(m_CursorPos.first, m_CursorPos.second);
    
    while (!m_QueuedDrawWidgets.empty())
    {
        CWidget *w = m_QueuedDrawWidgets.front();
        
        m_QueuedDrawWidgets.pop_front();
        
        // Draw after widget has been removed from queue, this way the widget or childs from the
        // widget can queue themselves while being drawn.
        w->Draw();
        
        if (m_QueuedDrawWidgets.empty())
            DoUpdate(); // Commit real drawing after last widget
    }
    
    return true;
}

void CTUI::AddGroup(CGroup *g, bool activate)
{
    m_pWinManager->AddWidget(g);
    
    if (activate)
        ActivateGroup(g);
    else
    {
        g->SetFocus(false);
        g->RequestQueuedDraw();
    }
}

void CTUI::ActivateGroup(CGroup *g)
{
    m_pWinManager->LockWidget(g);
    g->ReqFocus();
    g->SetNextFocWidget(false);
    UpdateButtonBar();
}

int CTUI::GetColorPair(int fg, int bg)
{
    if (!has_colors())
        return COLOR_PAIR(0);
    
    TColorMap::iterator it = m_ColorPairs.find(fg);
    if (it != m_ColorPairs.end())
    {
        std::map<int, int>::iterator it2 = it->second.find(bg);
        if (it2 != it->second.end())
            return COLOR_PAIR(it2->second);
    }
    
    m_iCurColorPair++;
    InitPair(m_iCurColorPair, fg, bg);
    m_ColorPairs[fg][bg] = m_iCurColorPair;
    return COLOR_PAIR(m_iCurColorPair);
}

void CTUI::QueueDraw(CWidget *w)
{
    if (!w->Enabled())
        return;
    
    // Uninitialized widgets aren't allowed, because we want to know any parenting and they _should_ be
    // refreshed by their parent anyway
    if (!w->GetWin())
        return;
    
    bool done = false;
    while (!done)
    {
        done = true;
        for (std::deque<CWidget *>::iterator it=m_QueuedDrawWidgets.begin();
             it!=m_QueuedDrawWidgets.end(); it++)
        {
            if (*it == w)
            {
                // Move to end of queue
                m_QueuedDrawWidgets.erase(it);
                done = false;
                break;
            }
            else if (IsParent(*it, w))
                return; // Parent is being redrawn, which will draw all childs
            else if (IsChild(*it, w))
            {
                // Remove any childs
                m_QueuedDrawWidgets.erase(it);
                done = false;
                break;
            }
        }
    }
    
    m_QueuedDrawWidgets.push_back(w);
}

void CTUI::RemoveFromQueue(CWidget *w)
{
    std::deque<CWidget *>::iterator it=std::find(m_QueuedDrawWidgets.begin(), m_QueuedDrawWidgets.end(), w);
    if (it != m_QueuedDrawWidgets.end())
        m_QueuedDrawWidgets.erase(it);
}

void CTUI::QueueRefresh()
{
    WNOUTRefresh(GetRootWin());
}

}

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

#ifndef WIDGET_H
#define WIDGET_H

#include <string>
#include <vector>
#include <utility>

#include "ncurses.h"

namespace NNCurses {

typedef std::pair<chtype, chtype> TColorPair;
typedef std::pair<std::string, std::string> TButtonDescPair;
typedef std::vector<TButtonDescPair> TButtonDescList;

class CGroup;

class CWidget
{
    friend class CGroup;
    friend class CTUI;
    
    CGroup *m_pParent;
    WINDOW *m_pParentWin, *m_pNCursWin;
    int m_iX, m_iY, m_iWidth, m_iHeight;
    int m_iMinWidth, m_iMinHeight;
    
    bool m_bInitialized;
    bool m_bBox;
    bool m_bFocused;
    bool m_bEnabled;
    bool m_bSizeChanged;
    bool m_bDeleted;

    TColorPair m_FColors, m_DFColors; // Focused and DeFocused colors
    bool m_bColorsChanged, m_bSetFColors, m_bSetDFColors;
    
    void MoveWindow(int x, int y);

protected:
    enum { EVENT_CALLBACK, EVENT_DATACHANGED, EVENT_REQUPDATE, EVENT_REQQUEUEDDRAW, EVENT_REQFOCUS };
    
    CWidget(void);
    
    // Virtual Interface
    virtual void CoreDraw(void) { DrawWidget(); }
    virtual void DoDraw(void) { }
    virtual void CoreInit(void) { }
    
    virtual void CoreTouchSize(void) { }
    virtual void UpdateSize(void) { }
    
    virtual void UpdateColors(void) { }
    
    virtual void UpdateFocus(void) { }
    virtual bool CoreCanFocus(void) { return false; }
    
    virtual void UpdateEnabled(void) { }
    
    virtual bool CoreHandleKey(wchar_t) { return false; }
    virtual bool CoreHandleEvent(CWidget *, int) { return false; }
    
    virtual int CoreRequestWidth(void) { return GetMinWidth(); }
    virtual int CoreRequestHeight(void) { return GetMinHeight(); }
    
    virtual void CoreGetButtonDescs(TButtonDescList &) { }
    
    // Interface to be used by friends and derived classes
    void InitDraw(void);
    void RefreshWidget(void);
    void DrawWidget(void);
    void Draw(void);

    void Init(void);
    
    void SetSize(int x, int y, int w, int h);
    void TouchSize(void) { m_bSizeChanged = true; CoreTouchSize(); }

    void SetParent(CGroup *g) { m_pParent = g; m_pParentWin = NULL; }
    void SetParent(WINDOW *w) { m_pParent = NULL; m_pParentWin = w; }

    bool HandleKey(wchar_t key) { return CoreHandleKey(key); }
    bool HandleEvent(CWidget *emitter, int event) { return CoreHandleEvent(emitter, event); }
    void PushEvent(int type);
    
    void GetButtonDescs(TButtonDescList &list) { CoreGetButtonDescs(list); }
    
    void TouchColor(void) { m_bColorsChanged = true; }
    void SetColorAttr(TColorPair colors, bool e);
    
public:
    virtual ~CWidget(void);

    void SetFColors(int fg, int bg) { SetFColors(TColorPair(fg, bg)); }
    void SetFColors(TColorPair colors);
    void SetDFColors(int fg, int bg) { SetDFColors(TColorPair(fg, bg)); }
    void SetDFColors(TColorPair colors);
    TColorPair GetFColors(void) { return m_FColors; }
    TColorPair GetDFColors(void) { return m_DFColors; }
    
    int X(void) const { return m_iX; }
    int Y(void) const { return m_iY; }
    int Width(void) const { return m_iWidth; }
    int Height(void) const { return m_iHeight; }
    int GetMinWidth(void) { return m_iMinWidth; }
    int GetMinHeight(void) { return m_iMinHeight; }
    void SetMinWidth(int w) { m_iMinWidth = w; }
    void SetMinHeight(int h) { m_iMinHeight = h; }
    int RequestWidth(void) { return CoreRequestWidth(); }
    int RequestHeight(void) { return CoreRequestHeight(); }
    
    void RequestQueuedDraw(void);

    WINDOW *GetWin(void) { return m_pNCursWin; }
    WINDOW *GetParentWin(void);
    CGroup *GetParentWidget(void) { return m_pParent; }
    
    bool HasBox(void) const { return m_bBox; }
    void SetBox(bool b) { m_bBox = b; }
    
    bool Focused(void) { return m_bFocused; }
    bool CanFocus(void) { return CoreCanFocus(); }
    void SetFocus(bool f); // This function applies the focus, and must only be used internally or when disabling focus
    void ReqFocus(void) { PushEvent(EVENT_REQFOCUS); } // This function asks for focus, and should be used by the user
    
    void Enable(bool e);
    bool Enabled(void) { return m_bEnabled; }
    
    bool Deleted(void) const { return m_bDeleted; }
};


}

#endif


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

#ifndef BUTTON_H
#define BUTTON_H

#include "widget.h"
#include "box.h"

namespace NNCurses {

class CLabel;

class CButton: public CBox
{
    CLabel *m_pLabel;
    const int m_iExtraWidth; 
    
protected:
    virtual void DoDraw(void);
    virtual bool CoreCanFocus(void) { return true; }
    virtual void UpdateColors(void);
    virtual bool CoreHandleKey(wchar_t key);
    virtual void CoreGetButtonDescs(TButtonDescList &list);
    
    virtual int FieldX(void) const { return m_iExtraWidth/2; }
    virtual int FieldWidth(void) const { return Width()-FieldX()-(m_iExtraWidth/2); }
    
public:
    CButton(const std::string &title) : CBox(VERTICAL, false), m_pLabel(NULL), m_iExtraWidth(4) { SetText(title); }
    
    void SetText(const std::string &title);
};



}

#endif

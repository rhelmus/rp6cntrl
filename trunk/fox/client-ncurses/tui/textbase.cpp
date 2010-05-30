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
#include <ctype.h>

#include "utf8.h"
#include "tui.h"
#include "textbase.h"

namespace NNCurses {

// -------------------------------------
// Text Base Class
// -------------------------------------

CTextBase::CTextBase(bool c, bool w) : m_bCenter(c), m_bWrap(w), m_iMaxReqWidth(0),
                                       m_iMaxReqHeight(0), m_MaxChars(0), m_ClearChars(0),
                                       m_LongestLine(0), m_iCurWidth(0)
{
}

TSTLStrSize CTextBase::GetNextLine(const std::string &text, TSTLStrSize start, int width)
{
    TSTLStrSize length = text.length(), end = 0;
    
    if (m_bWrap)
    {
        end = start + GetMBLenFromW(text.substr(start, std::string::npos), width)-1;
        
        if (end < start) // Overflow
            end = length - 1;
        
        if (end >= length)
            end = length - 1;
        
        TSTLStrSize newline = text.find("\n", start);
        if (newline < end)
            end = newline;
        
        if (((end+1) != length) && !isspace(m_QueuedText[end]))
        {
            std::string sub = m_QueuedText.substr(start, (end-start)+1);
            TSTLStrSize pos = sub.find_last_of(" \t\n");
            
            if (pos != std::string::npos)
                end = start + pos;
        }
    }
    else
    {
        end = m_QueuedText.find("\n", start);
            
        if (end == std::string::npos)
            end = length-1;
    }
    
    return end;
}

void CTextBase::UpdateText(int width)
{
    TSTLStrSize length = m_QueuedText.length(), start = 0, end;
    
    if (!m_Lines.empty() && (m_Lines.back().rfind("\n") == std::string::npos))
    {
        // This makes adding text easier
        m_QueuedText = m_Lines.back() + m_QueuedText;
        m_Lines.pop_back();
    }
    
    while (start < length)
    {
        end = GetNextLine(m_QueuedText, start, width);
        
        TSTLStrSize newlen = (end-start)+1;
        m_Lines.push_back(m_QueuedText.substr(start, newlen));
        m_LongestLine = std::max(m_LongestLine, MBWidth(m_QueuedText.substr(start, newlen)));
        start = end + 1;
    }
    
    m_QueuedText.clear();
}

TSTLStrSize CTextBase::LineCount()
{
    assert(m_QueuedText.empty()); // UNDONE: Can't use this function while widget needs updating
    return m_Lines.size();
}

int CTextBase::CoreRequestWidth()
{
    int min = std::max(1, GetMinWidth());
    
    if (m_LongestLine)
        min = std::max(min, SafeConvert<int>(m_LongestLine));
    
    if (!m_QueuedText.empty())
    {
        TSTLStrSize longest = 0, start = 0, end, length = m_QueuedText.length();
        const int width = (m_bWrap && (m_iMaxReqWidth > 0)) ? m_iMaxReqWidth : SafeConvert<int>(MBWidth(m_QueuedText));
        while (start < length)
        {
            end = GetNextLine(m_QueuedText, start, width);
            if (end >= length-1)
                end = length-1;
            longest = std::max(longest, MBWidth(m_QueuedText.substr(start, (end - start)+1)));
            start = end + 1;
        }
        
        min = std::max(min, SafeConvert<int>(longest));
    }
    
    if ((m_iMaxReqWidth > 0) && (min > m_iMaxReqWidth))
        min = m_iMaxReqWidth;
    
    return min;
}

int CTextBase::CoreRequestHeight()
{
    int min = std::max(1, GetMinHeight());
    
    if (!m_Lines.empty())
        min = std::max(min, SafeConvert<int>(m_Lines.size()));
    
    if (!m_QueuedText.empty())
    {
        int lines = 0;
        const int width = RequestWidth();
        TSTLStrSize start = 0, end, length = m_QueuedText.length();
        
        while (start < length)
        {
            end = GetNextLine(m_QueuedText, start, width);
            lines++;
            start = end + 1;
        }
        
        min = std::max(min, lines);
    }
    
    if ((m_iMaxReqHeight > 0) && (min > m_iMaxReqHeight))
        min = m_iMaxReqHeight;
    
    return min;
}

void CTextBase::UpdateSize()
{
    if (Width() != m_iCurWidth)
    {
        // Big update....
        std::string oldqueue = m_QueuedText;
        m_QueuedText.clear();
        
        for (TLinesList::iterator it = m_Lines.begin(); it != m_Lines.end(); it++)
            m_QueuedText += *it;
        
        m_QueuedText += oldqueue;
        m_Lines.clear();
        m_iCurWidth = Width();
    }
}

void CTextBase::AddText(std::string t)
{
    ConvertTabs(t);
    m_QueuedText += t;
    
    if (m_MaxChars > 0)
    {
        TSTLStrSize curchars = utf8::distance(m_QueuedText.begin(), m_QueuedText.end());
        for (TLinesList::iterator it = m_Lines.begin(); it != m_Lines.end(); it++)
            curchars += utf8::distance(it->begin(), it->end());

        if (curchars > m_MaxChars)
        {
            bool checklongest = false;
            TSTLStrSize remchars = m_ClearChars + (curchars - m_MaxChars);
            
            for (TLinesList::iterator it = m_Lines.begin(); it != m_Lines.end();)
            {
                const TSTLStrSize chars = utf8::distance(it->begin(), it->end());
                
                if (MBWidth(*it) == m_LongestLine)
                    checklongest = true;

                if (remchars >= chars) // Remove complete line?
                {
                    it = m_Lines.erase(it);
                    remchars -= chars;
                    if (remchars == 0)
                        break;
                }
                else
                {
                    std::string::iterator end = it->begin();
                    utf8::advance(end, remchars - 1, it->end());
                    it->erase(it->begin(), end);
                    remchars = 0;
                    break;
                }
            }
            
            if (remchars > 0) // Didn't remove enough from processed text?
            {
                std::string::iterator end = m_QueuedText.begin();
                while ((end != m_QueuedText.end()) && (remchars > 0))
                {
                    utf8::next(end, m_QueuedText.end());
                    remchars--;
                }
                m_QueuedText.erase(m_QueuedText.begin(), end);
            }
            
            if (checklongest)
            {
                m_LongestLine = 0;
                for (TLinesList::iterator it = m_Lines.begin(); it != m_Lines.end(); it++)
                    m_LongestLine = std::max(m_LongestLine, MBWidth(*it));
            }
        }
    }
    
    PushEvent(EVENT_REQUPDATE);
}

void CTextBase::Clear()
{
    m_LongestLine = 0;
    m_Lines.clear();
    m_QueuedText.clear();
    PushEvent(EVENT_REQUPDATE);
}


}

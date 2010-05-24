/***************************************************************************
 *   Copyright (C) 2009 by Rick Helmus   *
 *   rhelmus_AT_gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef EDITOR_H
#define EDITOR_H

#include <QWidget>

#include "qeditor.h"
#include "qcodeedit.h"

class QAction;
class QFormatScheme;
class QKeyEvent;
class QLanguageFactory;
class QSignalMapper;
class QToolBar;

class CEditor: public QWidget
{
    Q_OBJECT

    static bool init;
    static QFormatScheme *formats;
    static QLanguageFactory *langFactory;

    QCodeEdit *editControl;
    QToolBar *toolBar;
    QSignalMapper *panelSignalMapper;

    void addPanel(const QString &name, QCodeEdit::Position pos, QString key="", bool add=false);
    QToolBar *createToolbars(void);

private slots:
    void updateWrap(bool e);
    void updateLineEnding(int le);
    void updatePanel(const QString &paneln);
    
public:
    CEditor(QWidget *parent = 0, Qt::WindowFlags f = 0);

    void setText(const QString &text);
    void insertTextAtCurrent(const QString &text);
    void indentNewLine(void);
    void load(const QString &file);
    void save(const QString &file);
    QToolBar *getToolBar(void) { return toolBar; }
    QEditor *editor(void) { return editControl->editor(); }

    static void loadSettings(const QList<QEditor *> &editList);
};

#endif

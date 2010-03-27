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

#ifndef TCPUTIL_H
#define TCPUTIL_H

#include <QByteArray>
#include <QDataStream>

class QTcpSocket;

class CTcpWriter
{
    QTcpSocket *socket;
    QByteArray block;
    QDataStream *dataStream;
    
public:
    CTcpWriter(QTcpSocket *s);
    ~CTcpWriter(void);

    void write(void);

    template <typename C> QDataStream &operator <<(const C &data)
    {
        return (*dataStream << data);
    }
};

#endif

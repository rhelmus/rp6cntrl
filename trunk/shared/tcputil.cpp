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

#include <QTcpSocket>

#include "shared.h"
#include "tcputil.h"

CTcpMsgComposer::CTcpMsgComposer(ETcpMessage msg)
{
    dataStream = new QDataStream(&block, QIODevice::WriteOnly);
    dataStream->setVersion(QDataStream::Qt_4_4);
    *dataStream << (quint32)0; // Reserve place for size
    *dataStream << (quint8)msg;
}

CTcpMsgComposer::~CTcpMsgComposer()
{
    delete dataStream;
}

CTcpMsgComposer::operator QByteArray()
{
    // Put in the size
    const quint64 pos = dataStream->device()->pos();
    dataStream->device()->seek(0);
    *dataStream << (quint32)(block.size() - sizeof(quint32));
    dataStream->device()->seek(pos); // Restore old position
    return block;
}


QMap<ETcpMessage, EDataType> tcpDataTypes;
void initTcpDataTypes()
{
    tcpDataTypes[TCP_STATE_SENSORS] = DATA_BYTE;
    tcpDataTypes[TCP_BASE_LEDS] = DATA_BYTE;
    tcpDataTypes[TCP_M32_LEDS] = DATA_BYTE;
    tcpDataTypes[TCP_LIGHT_LEFT] = DATA_WORD;
    tcpDataTypes[TCP_LIGHT_RIGHT] = DATA_WORD;
    tcpDataTypes[TCP_MOTOR_SPEED_LEFT] = DATA_BYTE;
    tcpDataTypes[TCP_MOTOR_SPEED_RIGHT] = DATA_BYTE;
    tcpDataTypes[TCP_MOTOR_DESTSPEED_LEFT] = DATA_BYTE;
    tcpDataTypes[TCP_MOTOR_DESTSPEED_RIGHT] = DATA_BYTE;
    tcpDataTypes[TCP_MOTOR_DIST_LEFT] = DATA_WORD;
    tcpDataTypes[TCP_MOTOR_DIST_RIGHT] = DATA_WORD;
    tcpDataTypes[TCP_MOTOR_DESTDIST_LEFT] = DATA_WORD;
    tcpDataTypes[TCP_MOTOR_DESTDIST_RIGHT] = DATA_WORD;
    tcpDataTypes[TCP_MOTOR_CURRENT_LEFT] = DATA_WORD;
    tcpDataTypes[TCP_MOTOR_CURRENT_RIGHT] = DATA_WORD;
    tcpDataTypes[TCP_MOTOR_DIRECTIONS] = DATA_BYTE;
    tcpDataTypes[TCP_BATTERY] = DATA_WORD;
    tcpDataTypes[TCP_ACS_POWER] = DATA_BYTE;
    tcpDataTypes[TCP_MIC] = DATA_WORD;
    tcpDataTypes[TCP_LASTRC5] = DATA_WORD;
    tcpDataTypes[TCP_SHARPIR] = DATA_BYTE;
}

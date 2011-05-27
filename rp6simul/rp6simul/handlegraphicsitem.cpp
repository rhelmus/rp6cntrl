#include "handlegraphicsitem.h"

#include <QBrush>
#include <QPen>

CHandleGraphicsItem::CHandleGraphicsItem(EHandlePosFlags pos,
                                         QGraphicsItem *parent)
    : QGraphicsRectItem(0.0, 0.0, 7.5, 7.5, parent), handlePos(pos)
{
    setBrush(Qt::green);
    setPen(Qt::NoPen);
    setAcceptedMouseButtons(Qt::LeftButton);
}


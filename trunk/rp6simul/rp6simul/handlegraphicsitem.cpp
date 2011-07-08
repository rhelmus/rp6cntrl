#include "handlegraphicsitem.h"
#include "robotscene.h"

#include <QBrush>
#include <QDebug>
#include <QPen>

CHandleGraphicsItem::CHandleGraphicsItem(EHandlePosFlags pos,
                                         QGraphicsItem *parent)
    : QGraphicsRectItem(0.0, 0.0, 8.5, 8.5, parent), handlePos(pos)
{
    setBrush(Qt::green);
    setPen(Qt::NoPen);
    setAcceptedMouseButtons(Qt::LeftButton);
    CRobotScene::fixedSizeItems << this;
}

CHandleGraphicsItem::~CHandleGraphicsItem()
{
    CRobotScene::fixedSizeItems.removeOne(this);
}

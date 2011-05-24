#include "basegraphicsitem.h"

#include <QtGui>

CBaseGraphicsItem::CBaseGraphicsItem(QGraphicsItem *parent)
    : QGraphicsObject(parent), isMovable(true), dragging(false)
{
    setFlag(ItemIsSelectable);
    setAcceptedMouseButtons(Qt::LeftButton);
    updateMouseCursor(false);
}

void CBaseGraphicsItem::updateMouseCursor(bool selected)
{
    if (!selected || !isMovable)
        setCursor(Qt::ArrowCursor);
    else
        setCursor(Qt::SizeAllCursor);
}

void CBaseGraphicsItem::alignToGrid(QPointF pos)
{
    // UNDONE: Grid settings
    // UNDONE: Optional for robot item
    const int gridsize = 10;
    int rx = (int)pos.x() % gridsize, ry = (int)pos.y() % gridsize;

    if (rx >= gridsize/2.0)
        pos.rx() += (gridsize - rx);
    else
        pos.rx() -= rx;

    if (ry >= gridsize/2.0)
        pos.ry() += (gridsize - ry);
    else
        pos.ry() -= ry;

    setPos(pos);
}

void CBaseGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (isMovable)
    {
        mouseDragPos = event->pos();
        oldPos = pos();
        dragging = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void CBaseGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (dragging)
    {
        QRectF r(boundingRect());
        const QPointF offset(mouseDragPos - r.center());
        r.moveCenter(mapToParent(event->pos() - offset));
        alignToGrid(r.topLeft());
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void CBaseGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (isMovable && dragging)
    {
        dragging = false;
        if (pos() != oldPos)
            emit posChanged();
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void CBaseGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;
    QAction *aligna = menu.addAction("Align to grid");
    QAction *dela = menu.addAction(QIcon("../resource/delete.png"), "Delete");
    QAction *a =menu.exec(event->screenPos());

    if (a == aligna)
        alignToGrid(pos());
    else if (a == dela)
        deleteLater();
}

QVariant CBaseGraphicsItem::itemChange(GraphicsItemChange change,
                                       const QVariant &value)
{
    if (change == ItemSelectedHasChanged)
    {
        const bool s = value.toBool();
        updateMouseCursor(s);
        setZValue(s ? 1.0 : 0.0);
    }

    return QGraphicsItem::itemChange(change, value);
}

void CBaseGraphicsItem::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *)
{
    if (option->state & QStyle::State_Selected)
    {
        painter->save();
        painter->setPen(QPen(Qt::black, 0.7, Qt::DashLine));
        painter->drawRect(boundingRect());
        painter->restore();
    }
}

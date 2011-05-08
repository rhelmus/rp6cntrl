#include "resizablegraphicsitem.h"

#include <QtGui>

CResizableGraphicsItem::CResizableGraphicsItem(QGraphicsItem *parent)
    : QGraphicsItem(parent), pressedHandlePos(HANDLE_NONE)
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    setCursor(Qt::OpenHandCursor);

    addHandle(HANDLE_LEFT);
    addHandle(HANDLE_RIGHT);
    addHandle(HANDLE_TOP);
    addHandle(HANDLE_BOTTOM);
    addHandle(HANDLE_LEFT | HANDLE_TOP);
    addHandle(HANDLE_LEFT | HANDLE_BOTTOM);
    addHandle(HANDLE_RIGHT | HANDLE_TOP);
    addHandle(HANDLE_RIGHT | HANDLE_BOTTOM);

    // UNDONE
    boundRect = QRectF(0.0, 0.0, 100.0, 100.0);
    // UNDONE
    adjustHandles();
}

void CResizableGraphicsItem::addHandle(int pos)
{
    QGraphicsRectItem *handle = new QGraphicsRectItem(0.0, 0.0, 10.0, 10.0, this);
    handle->setBrush(Qt::green);
    handle->setPen(Qt::NoPen);

    if ((pos == HANDLE_LEFT) || (pos == HANDLE_RIGHT))
        handle->setCursor(Qt::SizeHorCursor);
    else if ((pos == HANDLE_TOP) || (pos == HANDLE_BOTTOM))
        handle->setCursor(Qt::SizeVerCursor);
    else if ((pos == (HANDLE_LEFT | HANDLE_TOP)) ||
             (pos == (HANDLE_RIGHT | HANDLE_BOTTOM)))
        handle->setCursor(Qt::SizeFDiagCursor);
    else if ((pos == (HANDLE_LEFT | HANDLE_BOTTOM)) ||
             (pos == (HANDLE_RIGHT | HANDLE_TOP)))
        handle->setCursor(Qt::SizeBDiagCursor);

    handles[pos] = handle;
}

void CResizableGraphicsItem::adjustHandles()
{
    for (THandleList::iterator it=handles.begin(); it!= handles.end(); ++it)
    {
        QRectF hrect = it.value()->boundingRect();

        hrect.moveCenter(boundingRect().center());
        if (it.key() & HANDLE_LEFT)
            hrect.moveLeft(boundingRect().left());
        if (it.key() & HANDLE_RIGHT)
            hrect.moveRight(boundingRect().right());
        if (it.key() & HANDLE_TOP)
            hrect.moveTop(boundingRect().top());
        if (it.key() & HANDLE_BOTTOM)
            hrect.moveBottom(boundingRect().bottom());

        it.value()->setPos(hrect.topLeft());
    }
}

void CResizableGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    for (THandleList::iterator it=handles.begin(); it!= handles.end(); ++it)
    {
        if (it.value()->contains(it.value()->mapFromParent(event->pos())))
        {
            pressedHandlePos = it.key();
            break;
        }
    }

    setCursor(Qt::ClosedHandCursor);

    if (pressedHandlePos == HANDLE_NONE)
        QGraphicsItem::mousePressEvent(event);
}

void CResizableGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    pressedHandlePos = HANDLE_NONE;
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::mouseReleaseEvent(event);
}

void CResizableGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (pressedHandlePos == HANDLE_NONE)
    {
        QGraphicsItem::mouseMoveEvent(event);
        return;
    }

    prepareGeometryChange();

    const qreal minsize = 20.0;
    const QGraphicsItem *handle = handles[pressedHandlePos];
    const QPointF hpos = mapFromItem(handle,
                                     handle->boundingRect().center());
    const QRectF oldbound(boundRect);
    qreal mx = 0.0, my = 0.0;
    if (pressedHandlePos & HANDLE_LEFT)
    {
        mx = event->pos().x() - hpos.x();
        if (mx > (boundRect.right() - minsize))
            mx = 0.0;
        else
            boundRect.setRight(boundRect.right() - mx);
    }
    else if (pressedHandlePos & HANDLE_RIGHT)
    {
        if (event->pos().x() > (boundRect.left() + minsize))
            boundRect.setRight(event->pos().x());
    }

    if (pressedHandlePos & HANDLE_TOP)
    {
        my = event->pos().y() - hpos.y();
        if (my > (boundRect.bottom() - minsize))
            my = 0.0;
        else
            boundRect.setBottom(boundRect.bottom() - my);
    }
    else if (pressedHandlePos & HANDLE_BOTTOM)
    {
        if (event->pos().y() > (boundRect.top() + minsize))
            boundRect.setBottom(event->pos().y());
    }

    if ((mx != 0.0) || (my != 0.0))
        moveBy(mx, my);


    // UNDONE sub-childs?
    QList<QGraphicsItem *> children = childItems();
    const qreal sx = boundRect.width() / oldbound.width();
    const qreal sy = boundRect.height() / oldbound.height();
    foreach (QGraphicsItem *item, children)
    {
        bool ishandle = false;
        foreach(QGraphicsItem *h, handles)
        {
            if (item == h)
            {
                ishandle = true;
                break;
            }
        }

        if (!ishandle)
        {
            item->setTransform(item->transform().scale(sx, sy));
            item->setPos(item->pos().x() * sx, item->pos().y() * sy);
        }
    }

    adjustHandles();
}

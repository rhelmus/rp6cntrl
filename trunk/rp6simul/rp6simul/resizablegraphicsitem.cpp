#include "resizablegraphicsitem.h"

#include <QtGui>

CResizableGraphicsItem::CResizableGraphicsItem(QGraphicsItem *citem,
                                               QGraphicsItem *parent)
    : QGraphicsItem(parent), containingItem(citem),
      pressedHandlePos(HANDLE_NONE)
{
    citem->setParentItem(this);

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
    // UNDONE: Clean

    QRectF myrect(boundingRect());
    for (THandleList::iterator it=handles.begin(); it!= handles.end(); ++it)
    {
        QRectF hrect = it.value()->boundingRect();

        hrect.moveCenter(myrect.center());
        if (it.key() & HANDLE_LEFT)
            hrect.moveLeft(myrect.left());
        if (it.key() & HANDLE_RIGHT)
            hrect.moveRight(myrect.right());
        if (it.key() & HANDLE_TOP)
            hrect.moveTop(myrect.top());
        if (it.key() & HANDLE_BOTTOM)
            hrect.moveBottom(myrect.bottom());

        it.value()->setPos(hrect.topLeft());
        qDebug() << "setPos:" << hrect.topLeft();

        const qreal fpx = transform().mapRect(QRectF(0.0, 0.0, 1.0, 1.0)).width();
        const qreal fpy = transform().mapRect(QRectF(0.0, 0.0, 1.0, 1.0)).height();
        const qreal fix = it.value()->transform().mapRect(QRectF(0.0, 0.0, 1.0, 1.0)).width();
        const qreal fiy = it.value()->transform().mapRect(QRectF(0.0, 0.0, 1.0, 1.0)).height();

        qDebug() << "fpx/fpy/fix/fiy" << fpx << fpy << fix << fiy;

        QTransform tr(it.value()->transform());

        qreal tx, ty;
        if (it.key() & HANDLE_LEFT)
            tx = 0.0;
        if (it.key() & HANDLE_RIGHT)
            tx = it.value()->boundingRect().right();
        if (it.key() & HANDLE_TOP)
            ty = 0.0;
        if (it.key() & HANDLE_BOTTOM)
            ty = it.value()->boundingRect().bottom();

        tr.translate(tx, ty);
        tr.scale(1/(fpx*fix), 1/(fpy*fiy));
        tr.translate(-tx, -ty);
        it.value()->setTransform(tr);
    }
}

void CResizableGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        QGraphicsItem::mousePressEvent(event);
        return;
    }

    for (THandleList::iterator it=handles.begin(); it!= handles.end(); ++it)
    {
        if (it.value()->contains(it.value()->mapFromParent(event->pos())))
        {
            pressedHandlePos = it.key();
            break;
        }
    }

    // UNDONE
    setCursor(Qt::ClosedHandCursor);

    if (pressedHandlePos == HANDLE_NONE)
        QGraphicsItem::mousePressEvent(event);
}

void CResizableGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    pressedHandlePos = HANDLE_NONE;

    if (event->button() == Qt::LeftButton)
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

    const qreal minsize = 20.0;
    const QGraphicsItem *handle = handles[pressedHandlePos];
    const QPointF hpos = mapFromItem(handle,
                                     handle->boundingRect().center());
    const QRectF myrect(boundingRect());
    qreal sx = 1.0, sy = 1.0;
    qreal mx = 0.0, my = 0.0;
    qreal tx, ty;

    if (pressedHandlePos & HANDLE_LEFT)
    {
        tx = myrect.right();
        mx = event->pos().x() - hpos.x();
        if (mx <= (myrect.right() - minsize))
            sx = (myrect.right() - mx) / myrect.right();
    }
    else if (pressedHandlePos & HANDLE_RIGHT)
    {
        tx = 0.0;
        if (event->pos().x() > (myrect.left() + minsize))
            sx = (event->pos().x() / myrect.right());
    }

    if (pressedHandlePos & HANDLE_TOP)
    {
        ty = myrect.bottom();
        my = event->pos().y() - hpos.y();
        if (my <= (myrect.bottom() - minsize))
            sy = (myrect.bottom() - my) / myrect.bottom();
    }
    else if (pressedHandlePos & HANDLE_BOTTOM)
    {
        ty = 0.0;
        if (event->pos().y() > (myrect.top() + minsize))
            sy = (event->pos().y() / myrect.bottom());
    }

    if ((sx != 1.0) || (sy != 1.0))
    {
        qDebug() << "sx/sy/mx/my" << sx << sy << mx << my;
        QTransform tr(transform());
        tr.translate(tx, ty);
        tr.scale(sx, sy);
        tr.translate(-tx, -ty);
        setTransform(/*transform().scale(sx, sy)*/tr);
    }

    adjustHandles();
}

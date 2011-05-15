#include "resizablegraphicsitem.h"

#include <QtGui>

namespace {

qreal scaleFactorX(const QTransform &tr)
{
    return tr.mapRect(QRectF(0.0, 0.0, 1.0, 1.0)).width();
}

qreal scaleFactorY(const QTransform &tr)
{
    return tr.mapRect(QRectF(0.0, 0.0, 1.0, 1.0)).height();
}

}

CResizableGraphicsItem::CResizableGraphicsItem(QGraphicsItem *citem,
                                               QGraphicsItem *parent)
    : QGraphicsItem(parent), containingItem(citem), pressedHandle(0)
{
    citem->setParentItem(this);

    setFlags(ItemIsSelectable);
    setCursor(Qt::OpenHandCursor);
    setAcceptedMouseButtons(Qt::LeftButton);

    addHandle(CHandleGraphicsItem::HANDLE_LEFT);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT);
    addHandle(CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_BOTTOM);
    addHandle(CHandleGraphicsItem::HANDLE_LEFT | CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_LEFT | CHandleGraphicsItem::HANDLE_BOTTOM);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT | CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT | CHandleGraphicsItem::HANDLE_BOTTOM);

    adjustHandles();
}

void CResizableGraphicsItem::addHandle(CHandleGraphicsItem::EHandlePosFlags pos)
{
    QGraphicsRectItem *handle = new CHandleGraphicsItem(pos, this);
    handle->hide();

    if ((pos == CHandleGraphicsItem::HANDLE_LEFT) ||
        (pos == CHandleGraphicsItem::HANDLE_RIGHT))
        handle->setCursor(Qt::SizeHorCursor);
    else if ((pos == CHandleGraphicsItem::HANDLE_TOP) ||
             (pos == CHandleGraphicsItem::HANDLE_BOTTOM))
        handle->setCursor(Qt::SizeVerCursor);
    else if ((pos == (CHandleGraphicsItem::HANDLE_LEFT | CHandleGraphicsItem::HANDLE_TOP)) ||
             (pos == (CHandleGraphicsItem::HANDLE_RIGHT | CHandleGraphicsItem::HANDLE_BOTTOM)))
        handle->setCursor(Qt::SizeFDiagCursor);
    else if ((pos == (CHandleGraphicsItem::HANDLE_LEFT | CHandleGraphicsItem::HANDLE_BOTTOM)) ||
             (pos == (CHandleGraphicsItem::HANDLE_RIGHT | CHandleGraphicsItem::HANDLE_TOP)))
        handle->setCursor(Qt::SizeBDiagCursor);

    handles[pos] = handle;
}

void CResizableGraphicsItem::adjustHandles()
{
    const QRectF myrect(boundingRect());
    const qreal fpx = scaleFactorX(transform());
    const qreal fpy = scaleFactorY(transform());

    for (THandleList::iterator it=handles.begin(); it!= handles.end(); ++it)
    {
        QRectF hrect = it.value()->boundingRect();
        qreal tx, ty;

        hrect.moveCenter(myrect.center());

        if (it.key() & CHandleGraphicsItem::HANDLE_LEFT)
        {
            hrect.moveLeft(myrect.left());
            tx = 0.0;
        }
        else if (it.key() & CHandleGraphicsItem::HANDLE_RIGHT)
        {
            hrect.moveRight(myrect.right());
            tx = it.value()->boundingRect().right();
        }
        if (it.key() & CHandleGraphicsItem::HANDLE_TOP)
        {
            hrect.moveTop(myrect.top());
            ty = 0.0;
        }
        else if (it.key() & CHandleGraphicsItem::HANDLE_BOTTOM)
        {
            hrect.moveBottom(myrect.bottom());
            ty = it.value()->boundingRect().bottom();
        }

        it.value()->setPos(hrect.topLeft());

        const qreal fix = scaleFactorX(it.value()->transform());
        const qreal fiy = scaleFactorY(it.value()->transform());
        QTransform tr(it.value()->transform());

        tr.translate(tx, ty);
        tr.scale(1/(fpx*fix), 1/(fpy*fiy));
        tr.translate(-tx, -ty);
        it.value()->setTransform(tr);
    }
}

void CResizableGraphicsItem::updateGeometry(const QPointF &mousepos)
{
    Q_ASSERT(pressedHandle);

    const qreal minsize = 20.0;
    const QRectF myrect(boundingRect());
    const QPointF hpos = mapFromItem(pressedHandle,
                                     pressedHandle->boundingRect().center());
    const CHandleGraphicsItem::EHandlePosFlags handlepos =
            pressedHandle->handlePosition();
    qreal sx = 1.0, sy = 1.0;
    qreal mx = 0.0, my = 0.0;
    qreal tx, ty;

    if (handlepos & CHandleGraphicsItem::HANDLE_LEFT)
    {
        tx = myrect.right();
        mx = mousepos.x() - hpos.x();
        if (mx <= (myrect.right() - minsize))
            sx = (myrect.right() - mx) / myrect.right();
    }
    else if (handlepos & CHandleGraphicsItem::HANDLE_RIGHT)
    {
        tx = 0.0;
        if (mousepos.x() > (myrect.left() + minsize))
            sx = (mousepos.x() / myrect.right());
    }

    if (handlepos & CHandleGraphicsItem::HANDLE_TOP)
    {
        ty = myrect.bottom();
        my = mousepos.y() - hpos.y();
        if (my <= (myrect.bottom() - minsize))
            sy = (myrect.bottom() - my) / myrect.bottom();
    }
    else if (handlepos & CHandleGraphicsItem::HANDLE_BOTTOM)
    {
        ty = 0.0;
        if (mousepos.y() > (myrect.top() + minsize))
            sy = (mousepos.y() / myrect.bottom());
    }

    if ((sx != 1.0) || (sy != 1.0))
    {
        if (((scaleFactorX(transform()) * sx * myrect.width()) <= minsize) ||
            ((scaleFactorY(transform()) * sy * myrect.height()) <= minsize))
            return;

        QTransform tr(transform());
        tr.translate(tx, ty);
        tr.scale(sx, sy);
        tr.translate(-tx, -ty);
        setTransform(tr);
    }

    adjustHandles();
}

void CResizableGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    setCursor(Qt::ClosedHandCursor);
    lastMousePos = event->pos();
    QGraphicsItem::mousePressEvent(event);
}

void CResizableGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QRectF r(mapToParent(boundingRect()).boundingRect());
    const QPointF offset(mapToParent(lastMousePos) - r.center());
    r.moveCenter(mapToParent(event->pos()) - offset);

    // Align to grid
    QPointF npos(r.topLeft());
    const int gridsize = 10;
    int rx = (int)npos.x() % gridsize, ry = (int)npos.y() % gridsize;

    if (rx >= gridsize/2.0)
        npos.rx() += (gridsize - rx);
    else
        npos.rx() -= rx;

    if (ry >= gridsize/2.0)
        npos.ry() += (gridsize - ry);
    else
        npos.ry() -= ry;

    setPos(npos);

    QGraphicsItem::mouseMoveEvent(event);
}

void CResizableGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::mouseReleaseEvent(event);
}

bool CResizableGraphicsItem::sceneEventFilter(QGraphicsItem *watched,
                                              QEvent *event)
{
    if (event->type() == QEvent::GraphicsSceneMousePress)
    {
        pressedHandle = dynamic_cast<CHandleGraphicsItem *>(watched);
        Q_ASSERT(pressedHandle);
    }
    else if (event->type() == QEvent::GraphicsSceneMouseRelease)
        pressedHandle = 0;
    else if (event->type() == QEvent::GraphicsSceneMouseMove)
    {
        QGraphicsSceneMouseEvent *me =
                static_cast<QGraphicsSceneMouseEvent*>(event);
        updateGeometry(mapFromItem(watched, me->pos()));
    }

    return false; // Propegate
}

QVariant CResizableGraphicsItem::itemChange(GraphicsItemChange change,
                                            const QVariant &value)
{
    if (change == ItemSceneHasChanged)
    {
        foreach(QGraphicsItem *h, handles)
            h->installSceneEventFilter(this);
    }
    else if (change == ItemSelectedHasChanged)
    {
        foreach(QGraphicsItem *h, handles)
            h->setVisible(value.toBool());
    }

    return QGraphicsItem::itemChange(change, value);
}

void CResizableGraphicsItem::setSize(qreal w, qreal h)
{
    const QRectF brect(boundingRect());
    const QPointF c(brect.center());
    QTransform tr(transform());

    const qreal cursx = scaleFactorX(tr), cursy = scaleFactorY(tr);
    const qreal sx = (w / brect.width()) / cursx;
    const qreal sy = (h / brect.height()) / cursy;

    tr.scale(sx, sy);
    setTransform(tr);
    adjustHandles();
}

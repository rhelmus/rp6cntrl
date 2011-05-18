#include "resizablepixmapgraphicsitem.h"

#include <QtGui>

CResizablePixmapGraphicsItem::CResizablePixmapGraphicsItem(const QPixmap &pm,
                                                           QGraphicsItem *parent)
    : QGraphicsItem(parent), pixmap(pm), pressedHandle(0), hasShape(false)
{
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

void CResizablePixmapGraphicsItem::addHandle(CHandleGraphicsItem::EHandlePosFlags pos)
{
    CHandleGraphicsItem *handle = new CHandleGraphicsItem(pos, this);
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

void CResizablePixmapGraphicsItem::adjustHandles()
{
    for (THandleList::iterator it=handles.begin(); it!= handles.end(); ++it)
    {
        QRectF hrect = it.value()->boundingRect();
        hrect.moveCenter(boundRect.center());

        if (it.key() & CHandleGraphicsItem::HANDLE_LEFT)
            hrect.moveLeft(boundRect.left());
        else if (it.key() & CHandleGraphicsItem::HANDLE_RIGHT)
            hrect.moveRight(boundRect.right());
        if (it.key() & CHandleGraphicsItem::HANDLE_TOP)
            hrect.moveTop(boundRect.top());
        else if (it.key() & CHandleGraphicsItem::HANDLE_BOTTOM)
            hrect.moveBottom(boundRect.bottom());

        it.value()->setPos(hrect.topLeft());
    }
}

void CResizablePixmapGraphicsItem::updateGeometry(const QPointF &mousepos)
{
    Q_ASSERT(pressedHandle);

    const CHandleGraphicsItem::EHandlePosFlags handlepos =
            pressedHandle->handlePosition();
    const qreal minsize = 20.0;
    qreal mvx = 0.0, mvy = 0.0, expx = 0.0, expy = 0.0;

    if (handlepos & CHandleGraphicsItem::HANDLE_LEFT)
    {
        if ((boundRect.width() - mousepos.x()) > minsize)
        {
            mvx = mousepos.x();
            expx = -mousepos.x();
        }
    }
    else if (handlepos & CHandleGraphicsItem::HANDLE_RIGHT)
    {
        if (mousepos.x() > minsize)
            expx = mousepos.x() - boundRect.right();
    }

    if (handlepos & CHandleGraphicsItem::HANDLE_TOP)
    {
        if ((boundRect.height() - mousepos.y()) > minsize)
        {
            mvy = mousepos.y();
            expy = -mousepos.y();
        }
    }
    else if (handlepos & CHandleGraphicsItem::HANDLE_BOTTOM)
    {
        if (mousepos.y() > minsize)
            expy = mousepos.y() - boundRect.bottom();
    }

    if ((mvx != 0.0) || (mvy != 0.0))
        moveBy(mvx, mvy);

    if ((expx != 0.0) || (expy != 0.0))
    {
        prepareGeometryChange();
        boundRect.setRight(boundRect.right() + expx);
        boundRect.setBottom(boundRect.bottom() + expy);
    }

    adjustHandles();
    hasShape = false;
}

void CResizablePixmapGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    setCursor(Qt::ClosedHandCursor);
    lastMousePos = event->pos();
    QGraphicsItem::mousePressEvent(event);
}

void CResizablePixmapGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QRectF r(boundingRect());
    const QPointF offset(lastMousePos - r.center());
    r.moveCenter(mapToParent(event->pos() - offset));

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

void CResizablePixmapGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::mouseReleaseEvent(event);
}

void CResizablePixmapGraphicsItem::paint(QPainter *painter,
                                         const QStyleOptionGraphicsItem *option,
                                         QWidget *widget)
{
    painter->drawPixmap(boundRect.toRect(), pixmap);
}

bool CResizablePixmapGraphicsItem::sceneEventFilter(QGraphicsItem *watched,
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

QVariant CResizablePixmapGraphicsItem::itemChange(GraphicsItemChange change,
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

QPainterPath CResizablePixmapGraphicsItem::shape() const
{
    if (!hasShape)
    {
        hasShape = true;
        pmShape = QPainterPath();
        QBitmap mask = pixmap.scaled(boundRect.size().toSize()).mask();
        Q_ASSERT(!mask.isNull());
        if (!mask.isNull())
            pmShape.addRegion(QRegion(mask));
        else
            pmShape.addRect(boundRect);
    }

    return pmShape;
}

void CResizablePixmapGraphicsItem::setSize(const QSizeF &s)
{
    prepareGeometryChange();
    boundRect.setSize(s);
    adjustHandles();
    hasShape = false;
}

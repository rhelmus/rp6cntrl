#include "rotatablepixmapgraphicsitem.h"
#include "utils.h"

#include <QtGui>

CRotatablePixmapGraphicsItem::CRotatablePixmapGraphicsItem(const QPixmap &pm,
                                                           QGraphicsItem *parent)
    : CBaseGraphicsItem(parent), pressedHandle(0), hasShape(false)
{
    createHandles();
    setPixmap(pm);
}

CRotatablePixmapGraphicsItem::CRotatablePixmapGraphicsItem(QGraphicsItem *parent)
    : CBaseGraphicsItem(parent), pressedHandle(0), hasShape(false)
{
    createHandles();
}

void CRotatablePixmapGraphicsItem::addHandle(CHandleGraphicsItem::EHandlePosFlags pos)
{
    QGraphicsRectItem *handle = new CHandleGraphicsItem(pos, this);
    handle->hide();
    handle->setCursor(QPixmap("../resource/rotate.png"));
    handles[pos] = handle;
}

void CRotatablePixmapGraphicsItem::createHandles()
{
    addHandle(CHandleGraphicsItem::HANDLE_LEFT |
              CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_LEFT |
              CHandleGraphicsItem::HANDLE_BOTTOM);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT |
              CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT |
              CHandleGraphicsItem::HANDLE_BOTTOM);
}

void CRotatablePixmapGraphicsItem::adjustHandles()
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

void CRotatablePixmapGraphicsItem::paint(QPainter *painter,
                                         const QStyleOptionGraphicsItem *option,
                                         QWidget *widget)
{
    CBaseGraphicsItem::paint(painter, option, widget);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawPixmap(boundRect.toRect(), pixmap);
}

bool CRotatablePixmapGraphicsItem::sceneEventFilter(QGraphicsItem *watched,
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

        const QPointF start(mapToScene(boundingRect().center()));
        const QPointF end(mapToScene(mapFromItem(pressedHandle, me->pos())));
        const qreal mangle = toClockwiseAngle(QLineF(start, end).angle());

        if (mangle != rotation())
        {
            const qreal olda = rotation();
            setRotation(mangle);
            emit rotationChanged(olda);
        }
    }

    return false; // Propegate
}

QVariant CRotatablePixmapGraphicsItem::itemChange(GraphicsItemChange change,
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

    return CBaseGraphicsItem::itemChange(change, value);
}

QPainterPath CRotatablePixmapGraphicsItem::shape() const
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

void CRotatablePixmapGraphicsItem::setPixmap(const QPixmap &pm)
{
    pixmap = pm;
    if (boundingRect().isNull())
    {
        prepareGeometryChange();
        boundRect.setSize(pm.size()); // Default
    }
    adjustHandles();
    hasShape = false;
    setTransformOriginPoint(boundRect.center());
}

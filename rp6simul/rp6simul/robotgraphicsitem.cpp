#include "handlegraphicsitem.h"
#include "lightgraphicsitem.h"
#include "robotgraphicsitem.h"
#include "utils.h"

#include <QtGui>

namespace {

bool checkCollidingItems(const QList<QGraphicsItem *> &obstacles)
{
    foreach (QGraphicsItem *it, obstacles)
    {
        if (!qgraphicsitem_cast<CLightGraphicsItem *>(it) &&
            !qgraphicsitem_cast<CHandleGraphicsItem *>(it))
            return true;
    }

    return false;
}

}

CRobotGraphicsItem::CRobotGraphicsItem(QGraphicsItem *parent)
    : CResizablePixmapGraphicsItem(parent),
      leftPower(0), rightPower(0), skipFrames(0), pressedHandle(0)
{
    QPixmap pm("../resource/rp6-top.png");
    origRobotSize = pm.size();
    setPixmap(pm.scaledToWidth(120, Qt::SmoothTransformation), false);

    setTransformOriginPoint(boundingRect().center());
    setResizable(false);
    setDeletable(false);
    setSnapsToGrid(false);

    addHandle(CHandleGraphicsItem::HANDLE_LEFT |
              CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_LEFT |
              CHandleGraphicsItem::HANDLE_BOTTOM);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT |
              CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT |
              CHandleGraphicsItem::HANDLE_BOTTOM);
}

void CRobotGraphicsItem::addHandle(CHandleGraphicsItem::EHandlePosFlags pos)
{
    QGraphicsRectItem *handle = new CHandleGraphicsItem(pos, this);
    handle->hide();
    handle->setCursor(QPixmap("../resource/rotate.png"));

    QRectF hrect = handle->boundingRect();
    const QRectF myrect(boundingRect());

    hrect.moveCenter(myrect.center());

    if (pos & CHandleGraphicsItem::HANDLE_LEFT)
        hrect.moveLeft(myrect.left());
    else if (pos & CHandleGraphicsItem::HANDLE_RIGHT)
        hrect.moveRight(myrect.right());
    if (pos & CHandleGraphicsItem::HANDLE_TOP)
        hrect.moveTop(myrect.top());
    else if (pos & CHandleGraphicsItem::HANDLE_BOTTOM)
        hrect.moveBottom(myrect.bottom());

    handle->setPos(hrect.topLeft());

    handles[pos] = handle;
}

QPointF CRobotGraphicsItem::mapDeltaPos(qreal x, qreal y) const
{
    return (mapToParent(x, y) - mapToParent(0.0, 0.0));
}

void CRobotGraphicsItem::tryMove(float lpower, float rpower)
{
    const float lspeed = lpower / 10.0;
    const float rspeed = rpower / 10.0;
    const float movespeed = (lspeed + rspeed) / 5.0;
    const float rotspeed = (lspeed - rspeed) / 15.0;

    if (!tryDoMove(rotspeed, mapDeltaPos(0.0, -movespeed)))
    {
        skipFrames = 3;

        const float l = 0.25, h = 0.75, inc = 0.25;
        for (float dx = l; dx <= h; dx += inc)
        {
            if (tryDoMove(0.0, mapDeltaPos(dx, 0.0)))
                return;

            if (tryDoMove(0.0, mapDeltaPos(-dx, 0.0)))
                return;

            for (float dy = l; dy <= h; dy += inc)
            {
                if (tryDoMove(0.0, mapDeltaPos(0.0, dy)))
                    return;
                if (tryDoMove(0.0, mapDeltaPos(0.0, -dy)))
                    return;
                if (tryDoMove(0.0, mapDeltaPos(dx, dy)))
                    return;
                if (tryDoMove(0.0, mapDeltaPos(-dx, dy)))
                    return;
            }
        }
    }

    skipFrames = 0;
}

bool CRobotGraphicsItem::tryDoMove(float rotspeed, QPointF dpos)
{
    const QPointF oldpos(pos());
    const qreal oldrot = rotation();

    setRotation(rotation() + rotspeed);
    if (checkCollidingItems(collidingItems()))
    {
        setRotation(oldrot);
        return false;
    }
    else
    {
        setPos(pos() + dpos);
        if (checkCollidingItems(collidingItems()))
        {
            setPos(oldpos);
            return false;
        }
    }

    return true;
}

void CRobotGraphicsItem::advance(int phase)
{
    if (!phase)
        return;

    if (skipFrames)
    {
        skipFrames--;
        return;
    }

    const QPointF prepos(pos());
    tryMove(leftPower, rightPower);

    if (pos() != prepos)
        emit posChanged(prepos);
}

bool CRobotGraphicsItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
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
        // 90.0 - ... : Convert from counter-clockwise + 90 to clockwise + 0
        const qreal mangle = 90.0 - QLineF(start, end).angle();

        if (mangle != rotation())
        {
            const qreal olda = rotation();
            setRotation(mangle);
            emit rotationChanged(olda);
        }
    }

    return false; // Propegate
}

QVariant CRobotGraphicsItem::itemChange(GraphicsItemChange change,
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

    return CResizablePixmapGraphicsItem::itemChange(change, value);
}

void CRobotGraphicsItem::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *widget)
{
    CResizablePixmapGraphicsItem::paint(painter, option, widget);

#if 0
    // UNDONE
    QPointF c(boundingRect().center());
    QLineF line(c.x(), 0.0, c.x(), -100.0);

    QPolygonF poly = mapToScene(line.x1(), line.y1(), line.x2(), line.y2());
    QList<QGraphicsItem *> obstacles = scene()->items(poly);

    foreach (QGraphicsItem *ob, obstacles)
    {
        if (ob == this)
            continue;
        QPolygonF obpoly(mapFromItem(ob, ob->boundingRect()));
        for (QPolygonF::iterator it=obpoly.begin(); it!=(obpoly.end()-1); ++it)
        {
            QLineF obl(*it, *(it+1));
            QPointF interp;
            if (line.intersect(obl, &interp) == QLineF::BoundedIntersection)
            {
                QLineF tmp(line.p1(), interp);
                if (tmp.length() < line.length())
                    line.setP2(interp);
            }
        }

    }

    painter->drawLine(line);
#endif
}

void CRobotGraphicsItem::drawLEDs(QPainter *painter) const
{
    const QTransform tr(sceneTransform());

    // Equal aspect ratio, so scaling equals for width and height
    const qreal scale = boundingRect().width() / (qreal)origRobotSize.width();

    if (enabledLEDs[LED1])
        drawLED(*painter, "led1", tr, scale);
    if (enabledLEDs[LED2])
        drawLED(*painter, "led2", tr, scale);
    if (enabledLEDs[LED3])
        drawLED(*painter, "led3", tr, scale);
    if (enabledLEDs[LED4])
        drawLED(*painter, "led4", tr, scale);
    if (enabledLEDs[LED5])
        drawLED(*painter, "led5", tr, scale);
    if (enabledLEDs[LED6])
        drawLED(*painter, "led6", tr, scale);
}

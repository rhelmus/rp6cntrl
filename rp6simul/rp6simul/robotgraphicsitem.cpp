#include "robotgraphicsitem.h"

#include <QtGui>

CRobotGraphicsItem::CRobotGraphicsItem() : leftPower(0),
    rightPower(0), skipFrames(0)
{
    setPixmap(QPixmap("../resource/rp6-top.png"));
    setTransformOriginPoint(boundingRect().center());
    setTransformationMode(Qt::SmoothTransformation);
}

QPointF CRobotGraphicsItem::mapDeltaPos(qreal x, qreal y) const
{
    return (mapToParent(x, y) - mapToParent(0.0, 0.0));
}

bool CRobotGraphicsItem::tryMove(float lpower, float rpower)
{
    const float lspeed = lpower / 10.0;
    const float rspeed = rpower / 10.0;
    const float movespeed = (lspeed + rspeed) / 5.0;
    const float rotspeed = (lspeed - rspeed) / 15.0;

    QList<QGraphicsItem *> obstacles =
            tryDoMove(rotspeed, mapDeltaPos(0.0, -movespeed));

    if (!obstacles.isEmpty())
    {
        skipFrames = 3;

        const float l = 0.25, h = 0.75, inc = 0.25;
        for (float dx = l; dx <= h; dx += inc)
        {
            if (tryDoMove(0.0, mapDeltaPos(dx, 0.0)).isEmpty())
                return true;

            if (tryDoMove(0.0, mapDeltaPos(-dx, 0.0)).isEmpty())
                return true;

            for (float dy = l; dy <= h; dy += inc)
            {
                if (tryDoMove(0.0, mapDeltaPos(0.0, dy)).isEmpty())
                    return true;
                if (tryDoMove(0.0, mapDeltaPos(0.0, -dy)).isEmpty())
                    return true;
                if (tryDoMove(0.0, mapDeltaPos(dx, dy)).isEmpty())
                    return true;
                if (tryDoMove(0.0, mapDeltaPos(-dx, dy)).isEmpty())
                    return true;
            }
        }
    }

    skipFrames = 0;

    return (!obstacles.isEmpty());
}

QList<QGraphicsItem *> CRobotGraphicsItem::tryDoMove(float rotspeed, QPointF dpos)
{
    const QPointF oldpos(pos());
    const qreal oldrot = rotation();

    setRotation(rotation() + rotspeed);
    QList<QGraphicsItem *> ret = collidingItems();
    if (!ret.isEmpty())
    {
        setRotation(oldrot);
    }
    else
    {
        setPos(pos() + dpos);

        ret = collidingItems();

        if (!ret.isEmpty())
            setPos(oldpos);
    }

    return ret;
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

    tryMove(leftPower, rightPower);
}

void CRobotGraphicsItem::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *widget)
{
    QGraphicsPixmapItem::paint(painter, option, widget);

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
}

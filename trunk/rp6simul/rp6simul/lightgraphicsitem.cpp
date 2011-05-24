#include "handlegraphicsitem.h"
#include "lightgraphicsitem.h"

#include <QtGui>

CLightGraphicsItem::CLightGraphicsItem(float r, QGraphicsItem *parent) :
    CBaseGraphicsItem(parent), radius(r), handleDragging(false)
{
    setFlag(ItemIsSelectable);

    radiusHandle = new CHandleGraphicsItem(CHandleGraphicsItem::HANDLE_CENTER,
                                           this);
    radiusHandle->setVisible(false);
    radiusHandle->setCursor(Qt::SizeVerCursor);
    setRadiusHandlePos();
}

void CLightGraphicsItem::setRadiusHandlePos()
{
    const qreal w = radiusHandle->boundingRect().width();
    const qreal h = radiusHandle->boundingRect().height();
    radiusHandle->setPos(radius - (w / 2.0), radius - (h / 2.0));
}

float CLightGraphicsItem::intensityAt(const QPointF &p,
                          const QList<QPolygonF> &obstacles) const
{
    const QPointF c(pos() + QPointF(radius, radius));
    const QLineF line(c, p);
    const float d = line.length();
    if (d > radius)
        return 0.0;

    foreach (QPolygonF obpoly, obstacles)
    {
        if (obpoly.containsPoint(c, Qt::OddEvenFill) ||
            obpoly.containsPoint(p, Qt::OddEvenFill))
            return 0.0;

        for (QPolygonF::iterator it=obpoly.begin(); it!=(obpoly.end()-1); ++it)
        {
            QLineF obl(*it, *(it+1));
            if (line.intersect(obl, 0) == QLineF::BoundedIntersection)
                return 0.0;
        }
    }

    const float maxint = 2.0;
    return ((radius - d) / radius) * maxint;
}

bool CLightGraphicsItem::sceneEventFilter(QGraphicsItem *, QEvent *event)
{
    if (event->type() == QEvent::GraphicsSceneMousePress)
    {
        handleDragging = true;
        QGraphicsSceneMouseEvent *me =
                static_cast<QGraphicsSceneMouseEvent*>(event);
        radiusDragMousePos = mapToScene(mapFromItem(radiusHandle, me->pos()));
        oldRadius = radius;
    }
    else if (event->type() == QEvent::GraphicsSceneMouseRelease)
    {
        if (handleDragging)
        {
            handleDragging = false;
            if (oldRadius != radius)
                emit radiusChanged();
        }
    }
    else if (event->type() == QEvent::GraphicsSceneMouseMove)
    {
        if (handleDragging)
        {
            QGraphicsSceneMouseEvent *me =
                    static_cast<QGraphicsSceneMouseEvent*>(event);

            const QPointF mp(mapToScene(mapFromItem(radiusHandle, me->pos())));
            const QPointF c(mapToParent(boundingRect().center()));

            prepareGeometryChange();

            radius += (radiusDragMousePos.y() - mp.y());
            if (radius < 20.0)
                radius = 20.0;

            QRectF r(boundingRect());
            r.moveCenter(c);
            setPos(r.topLeft());

            setRadiusHandlePos();

            radiusDragMousePos = mp;
        }
    }

    return false; // Propegate
}

QVariant CLightGraphicsItem::itemChange(GraphicsItemChange change,
                                        const QVariant &value)
{
    if (change == ItemSceneHasChanged)
        radiusHandle->installSceneEventFilter(this);
    else if (change == ItemSelectedHasChanged)
        radiusHandle->setVisible(value.toBool());

    return CBaseGraphicsItem::itemChange(change, value);
}

QPainterPath CLightGraphicsItem::shape() const
{
    QPainterPath ret;
    ret.addEllipse(boundingRect());
    return ret;
}

void CLightGraphicsItem::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *widget)
{
    CBaseGraphicsItem::paint(painter, option, widget);

    QRadialGradient g(radius, radius, radius);
    g.setColorAt(0.0, QColor(255, 255, 0, 200));
    g.setColorAt(1.0, QColor(255, 180, 0, 50));
    painter->setBrush(g);
    painter->drawEllipse(boundingRect());
}

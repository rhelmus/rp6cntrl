#include "robotscene.h"

#include <QtGui>

float CLight::intensityAt(const QPointF &p) const
{
#if 0
    const qreal intensity = 300.0;
    const qreal quadraticIntensity = 150 * intensity;
    const qreal linearIntensity = 30 * intensity;
    const qreal d = QLineF(pos, p).length() - intenseRadius();

    if (d < 0.0)
        return 255.0;

    return quadraticIntensity / (d * d)
            + linearIntensity / d;
#else
    const float d = QLineF(pos, p).length() - intenseRadius();
    const float maxintens = 255 - 80;

    if (d < 0.0)
        return maxintens;

    return qMax(0.0f, maxintens - ((d / (radius - intenseRadius())) * maxintens));
#endif
}


CRobotScene::CRobotScene(QObject *parent) :
    QGraphicsScene(parent)
{
}

void CRobotScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);

    painter->save();

    painter->setCompositionMode(QPainter::CompositionMode_Overlay);
    painter->setPen(Qt::NoPen);

    foreach (CLight l, lights)
    {
        const float rad = l.intenseRadius();
        QRadialGradient g(l.position(), rad);
        g.setColorAt(0.0, QColor(255, 255, 255));
        g.setColorAt(1.0, QColor(127, 125, 127));
        painter->setBrush(g);
        painter->drawEllipse(l.position(), rad, rad);
    }

    painter->restore();

    foreach (CLight l, lights)
    {
//        painter->drawEllipse(l.position(), l.fullRadius(), l.fullRadius());
    }
}

void CRobotScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    mousePos = event->scenePos();

    int mint = 80;
    foreach (CLight l, lights)
    {
        mint += qRound(l.intensityAt(mousePos));
    }

    qDebug() << "intensity@" << mousePos << "=" << mint;

    QGraphicsScene::mouseMoveEvent(event);
}

void CRobotScene::updateLighting()
{
    CLight l(lights[0]);
    for (float d = 0.0; d< 400.0; d+=2)
    {
        QPointF p = l.position() + QPointF(d, 0.0);
        qDebug() << "intensity @" << d << ":" << l.intensityAt(p);
    }

    qDebug() << "light size:" << sceneRect().size() << sceneRect();
    QImage image(sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);

    const int baseintensity = 80;
    for (int x=sceneRect().left(); x<=sceneRect().right(); ++x)
    {
        for (int y=sceneRect().top(); y<=sceneRect().bottom(); ++y)
        {
            int intensity = baseintensity;

            foreach (CLight l, lights)
                intensity += qRound(l.intensityAt(QPointF(x, y)));

            const int alpha = qBound(0, 255 - intensity, 255);
            painter.setPen(QPen(QBrush(QColor(0, 0, 0, alpha)), 1.0));
            painter.drawPoint(x, y);
        }
    }

    setForegroundBrush(QBrush(image));
}

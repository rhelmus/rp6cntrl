#include "robotscene.h"

#include <QtGui>

float CLight::intensityAt(const QPointF &p) const
{
    const float d = QLineF(pos, p).length() - intenseRadius();
    const float maxintens = 255 - 80;

    if (d < 0.0)
        return maxintens;

    return qMax(0.0f, maxintens - ((d / (radius - intenseRadius())) * maxintens));
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
}

void CRobotScene::updateLighting()
{
    QImage image(sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);

    const int baseintensity = 255;
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

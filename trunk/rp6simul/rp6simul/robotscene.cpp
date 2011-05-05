#include "robotscene.h"

#include <QtGui>

float CLight::intensityAt(const QPointF &p) const
{
    const qreal quadraticIntensity = 150 * intensity;
    const qreal linearIntensity = 30 * intensity;
    const qreal d = QLineF(pos, p).length();
    return quadraticIntensity / (d * d)
            + linearIntensity / d;
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
    painter->setBrush(QColor(200, 200, 200));
    painter->setPen(Qt::NoPen);

    foreach (CLight l, lights)
    {
        painter->drawEllipse(l.position(), 150.0, 150.0);
    }

    painter->restore();
}

void CRobotScene::updateLighting()
{
    qDebug() << "light size:" << sceneRect().size() << sceneRect();
    QImage pixmap(sceneRect().size().toSize(), QImage::Format_ARGB32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    const int baseintensity = 80;
    for (int x=sceneRect().left(); x<=sceneRect().right(); ++x)
    {
        for (int y=sceneRect().top(); y<=sceneRect().bottom(); ++y)
        {
            int intensity = baseintensity;

            foreach (CLight l, lights)
                intensity += l.intensityAt(QPointF(x, y));

            const int alpha = qMax(0, 255 - intensity);
            painter.setPen(QPen(QBrush(QColor(0, 0, 0, alpha)), 1.0));
            painter.drawPoint(x, y);
        }
    }

    setForegroundBrush(QBrush(pixmap));
}

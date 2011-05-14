#include "robotscene.h"

#include <QtGui>

float CLight::intensityAt(const QPointF &p) const
{
    const float d = QLineF(pos, p).length() - intenseRadius();
    const float maxintens = 255 - 80; // UNDONE

    if (d < 0.0)
        return maxintens;

    return qMax(0.0f, maxintens - ((d / (radius - intenseRadius())) * maxintens));
}


CRobotScene::CRobotScene(QObject *parent) :
    QGraphicsScene(parent), blockSize(30.0, 30.0),
    blockPixmap(QPixmap("../resource/wall.jpg").scaled(blockSize.toSize(),
                                                       Qt::IgnoreAspectRatio,
                                                       Qt::SmoothTransformation)),
    backGroundPixmap("../resource/floor.jpg")
{
    setBackgroundBrush(Qt::black);
}

void CRobotScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);

    painter->save();
    painter->setBrush(backGroundPixmap);
    painter->drawRect(sceneRect());
    painter->restore();
}

void CRobotScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);

    painter->save();

    painter->drawImage(0, 0, shadowImage);

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

    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter->setBrush(blockPixmap);
    foreach (QGraphicsItem *w, wallBlocks)
        painter->drawPolygon(w->mapToScene(w->boundingRect()));

    painter->restore();
}

void CRobotScene::addVertWall(const QPointF &pos, float h)
{
    QGraphicsRectItem *it;
    for (float y=pos.y(); y<h; y+=blockSize.height())
    {
        if ((y + blockSize.height()) > h)
            it = addRect(pos.x(), y, blockSize.width(), h);
        else
            it = addRect(pos.x(), y, blockSize.width(), blockSize.height());

        it->setPen(Qt::NoPen);
        wallBlocks << it;
    }
}

void CRobotScene::addHorizWall(const QPointF &pos, float w)
{
    QGraphicsRectItem *it;
    for (float x=pos.x(); x<w; x+=blockSize.width())
    {
        if ((x + blockSize.width()) > w)
            it = addRect(x, pos.y(), w, blockSize.height());
        else
            it = addRect(x, pos.y(), blockSize.width(), blockSize.height());

        it->setPen(Qt::NoPen);
        wallBlocks << it;
    }
}

void CRobotScene::updateShadows()
{
    shadowImage = QImage(sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    shadowImage.fill(Qt::transparent);
    QPainter painter(&shadowImage);

    const int baseintensity = 255; // UNDONE
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

    update();
}

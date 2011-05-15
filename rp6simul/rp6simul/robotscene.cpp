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
    backGroundPixmap("../resource/floor.jpg"), mouseMode(MODE_WALL)
{
    setBackgroundBrush(Qt::black);
}

QRectF CRobotScene::getDragRect() const
{
    QRectF ret;
    ret.setTopLeft(mouseDragStartPos);
    ret.setBottomRight(mousePos);
    return ret;
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
    foreach (QGraphicsItem *w, walls.keys())
        painter->drawPolygon(w->mapToScene(w->boundingRect()));

    if ((mouseMode == MODE_WALL) &&
            (QApplication::mouseButtons() == Qt::LeftButton))
    {
        painter->setOpacity(0.75);
        painter->setBrush(blockPixmap);
        painter->drawRect(getDragRect());
    }

    painter->restore();
}

void CRobotScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    mouseDragStartPos = event->scenePos();
    if (mouseMode == MODE_POINT)
        QGraphicsScene::mousePressEvent(event);
}

void CRobotScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    mousePos = event->scenePos();
    if (mouseMode == MODE_POINT)
        QGraphicsScene::mouseMoveEvent(event);
    else
        update(); // UNDONE
}

void CRobotScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (mouseMode == MODE_POINT)
        QGraphicsScene::mousePressEvent(event);
    else if (mouseMode == MODE_WALL)
    {
        addWall(getDragRect(), false);
    }
}

void CRobotScene::addWall(const QRectF &rect, bool st)
{
    QGraphicsRectItem *it;
    it = addRect(rect);
    it->setPen(Qt::NoPen);
    walls[it] = st;
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

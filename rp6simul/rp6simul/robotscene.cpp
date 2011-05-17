#include "robotscene.h"
#include "resizablegraphicsitem.h"
#include "rp6simul.h"

#include <QtGui>

float CLight::intensityAt(const QPointF &p,
                          const QList<QPolygonF> &obstacles) const
{
#if 0
    const float d = QLineF(pos, p).length() - intenseRadius();
    const float maxintens = 255 - 80; // UNDONE

    if (d < 0.0)
        return maxintens;

    return qMax(0.0f, maxintens - ((d / (radius - intenseRadius())) * maxintens));
#endif
    const QLineF line(pos, p);
    const float d = line.length();
    if (d > radius)
        return 0.0;

    foreach (QPolygonF obpoly, obstacles)
    {
        if (obpoly.containsPoint(pos, Qt::OddEvenFill) ||
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


CRobotScene::CRobotScene(QObject *parent) :
    QGraphicsScene(parent), blockSize(30.0, 30.0),
    blockPixmap(QPixmap("../resource/wall.jpg").scaled(blockSize.toSize(),
                                                       Qt::IgnoreAspectRatio,
                                                       Qt::SmoothTransformation)),
    boxPixmap("../resource/cardboard-box.png"),
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

    painter->setBrush(blockPixmap);
    foreach (QGraphicsItem *w, walls.keys())
        painter->drawPolygon(w->mapToScene(w->boundingRect()));

    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawImage(0, 0, shadowImage);

    painter->setCompositionMode(QPainter::CompositionMode_Overlay);
    painter->drawImage(0, 0, lightImage);

//    painter->setCompositionMode(QPainter::CompositionMode_Overlay);
    painter->setPen(Qt::NoPen);
#if 0
    foreach (CLight l, lights)
    {
        const float rad = l.intenseRadius();
        QRadialGradient g(l.position(), rad);
        g.setColorAt(0.0, QColor(255, 255, 255));
        g.setColorAt(1.0, QColor(127, 125, 127));
        painter->setBrush(g);
//        painter->setBrush(Qt::blue);

        QPolygonF poly(QRectF(l.position().x()-rad, l.position().y() - rad,
                              rad*2, rad*2));
//        painter->drawEllipse(l.position(), rad, rad);

        foreach (QGraphicsItem *w, walls.keys())
        {
            QPolygonF wpoly(w->mapToParent(w->boundingRect()));
            poly = poly.subtracted(wpoly);
        }

        painter->drawPolygon(poly);
        painter->setClipping(false);
    }

    painter->setClipping(false);
#endif
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (QApplication::mouseButtons() == Qt::LeftButton)
    {
        if (mouseMode == MODE_WALL)
        {
            painter->setOpacity(0.75);
            painter->setBrush(blockPixmap);
            painter->drawRect(getDragRect());
        }
        else if (mouseMode == MODE_BOX)
        {
            painter->setOpacity(0.75);
            painter->drawPixmap(getDragRect().toRect(), boxPixmap);
        }
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
    else if ((mouseMode == MODE_WALL) || (mouseMode == MODE_BOX) ||
             (mouseMode == MODE_LIGHT))
        update();
}

void CRobotScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (mouseMode == MODE_POINT)
        QGraphicsScene::mousePressEvent(event);
    else if (mouseMode == MODE_WALL)
    {
        addWall(getDragRect(), false);
        updateShadows();
    }
    else if (mouseMode == MODE_BOX)
    {
        QGraphicsPixmapItem *p = new QGraphicsPixmapItem(boxPixmap);
        CResizableGraphicsItem *resi = new CResizableGraphicsItem(p);
        const QRectF r(getDragRect());
        resi->setPos(r.topLeft());
        resi->setSize(r.size());
        addItem(resi);
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
    QPainter shpainter(&shadowImage);

    lightImage = QImage(sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    lightImage.fill(Qt::transparent);
    QPainter lipainter(&lightImage);

    const float baseintensity = 0.35; // UNDONE

    QProgressDialog prgdialog(CRP6Simulator::getInstance(),
                              Qt::FramelessWindowHint);
    prgdialog.setLabelText("Calculating light map...");
    prgdialog.setRange(0, sceneRect().width());
    prgdialog.setMinimumDuration(50);
    prgdialog.setValue(0);
    prgdialog.setCancelButton(0); // No canceling

    QList<QPolygonF> obstacles;
    foreach (QGraphicsItem *w, walls.keys())
        obstacles << w->mapToScene(w->boundingRect());

    // UNDONE: Optimise (eg cache sceneRect coords)
    for (int x=sceneRect().left(); x<=sceneRect().right(); ++x)
    {
        prgdialog.setValue(x);
        if (!(x % 30))
            qApp->processEvents();

        for (int y=sceneRect().top(); y<=sceneRect().bottom(); ++y)
        {
            float intensity = baseintensity;

            foreach (CLight l, lights)
            {
                float i = l.intensityAt(QPointF(x, y), obstacles);
                if (i > baseintensity)
                    intensity += (i - baseintensity);
            }

            if (intensity > 1.0)
            {
                const int c = qRound(intensity * 127.0);
                lipainter.setPen(QColor(c, c, c));
                lipainter.drawPoint(x, y);
            }
            else
            {
                const int alpha = 255 - qRound(intensity * 255.0);
                shpainter.setPen(QPen(QBrush(QColor(0, 0, 0, alpha)), 1.0));
                shpainter.drawPoint(x, y);
            }
        }
    }

#if 0
    const int baseintensity = 80; // UNDONE
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
#endif
    update();
}

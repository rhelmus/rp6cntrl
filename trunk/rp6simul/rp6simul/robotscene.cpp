#include "robotscene.h"
#include "resizablepixmapgraphicsitem.h"
#include "rp6simul.h"

#include <QtGui>

float CLight::intensityAt(const QPointF &p,
                          const QList<QPolygonF> &obstacles) const
{
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
    backGroundPixmap(QPixmap("../resource/floor.jpg").scaled(300, 300,
                                                             Qt::IgnoreAspectRatio,
                                                             Qt::SmoothTransformation)),
    lightingDirty(false), dragging(false), mouseMode(MODE_POINT)
{
    setBackgroundBrush(Qt::darkGray);
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
    painter->drawTiledPixmap(sceneRect(), backGroundPixmap);
    painter->restore();
}

void CRobotScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);

    painter->save();

    if (!editModeEnabled)
    {
        painter->drawImage(0, 0, shadowImage);

        painter->setCompositionMode(QPainter::CompositionMode_Overlay);
        painter->drawImage(0, 0, lightImage);
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    if (dragging)
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
    if (!editModeEnabled || (mouseMode == MODE_POINT))
        QGraphicsScene::mousePressEvent(event);
    else if ((mouseMode == MODE_WALL) || (mouseMode == MODE_BOX) ||
             (mouseMode == MODE_LIGHT))
    {
        mouseDragStartPos = event->scenePos();
        dragging = true;
    }
}

void CRobotScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    mousePos = event->scenePos();
    if (!editModeEnabled || (mouseMode == MODE_POINT))
        QGraphicsScene::mouseMoveEvent(event);
    else if (dragging)
        update();
}

void CRobotScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (!editModeEnabled || (mouseMode == MODE_POINT))
        QGraphicsScene::mousePressEvent(event);
    else if (mouseMode == MODE_WALL)
    {
        addWall(getDragRect(), false);
        lightingDirty = true;
    }
    else if (mouseMode == MODE_BOX)
    {
        CResizablePixmapGraphicsItem *resi =
                new CResizablePixmapGraphicsItem(boxPixmap, false);
        const QRectF r(getDragRect());
        resi->setPos(r.topLeft());
        resi->setSize(r.size());
        addItem(resi);
    }

    dragging = false;
}

void CRobotScene::addWall(const QRectF &rect, bool st)
{
    if (!st)
    {
        CResizablePixmapGraphicsItem *resi
                = new CResizablePixmapGraphicsItem(blockPixmap, true);
        resi->setPos(rect.topLeft());
        resi->setSize(rect.size());
        addItem(resi);
        walls[resi] = st;
    }
    else
    {
        QGraphicsRectItem *it = new QGraphicsRectItem(0.0, 0.0,
                                                      rect.width(),
                                                      rect.height());
        it->setPen(Qt::NoPen);
        it->setBrush(blockPixmap);
        it->setPos(rect.topLeft());
        addItem(it);
        walls[it] = st;
    }
}

void CRobotScene::updateLighting()
{
    shadowImage = QImage(sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    shadowImage.fill(Qt::transparent);
    QPainter shpainter(&shadowImage);
    shpainter.setRenderHint(QPainter::Antialiasing);

    lightImage = QImage(sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    lightImage.fill(Qt::transparent);
    QPainter lipainter(&lightImage);
    lipainter.setRenderHint(QPainter::Antialiasing);

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

    for (int x=sceneRect().left(); x<=sceneRect().right(); ++x)
    {        
        if (!(x % 30))
        {
            prgdialog.setValue(x);
            qApp->processEvents();
        }

        for (int y=sceneRect().top(); y<=sceneRect().bottom(); ++y)
        {
            float intensity = baseintensity;

            foreach (CLight l, lights)
                intensity += l.intensityAt(QPointF(x, y), obstacles);

            if (intensity > 1.0)
            {
                intensity = qMin(intensity, 2.0f);
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

    update();
}

void CRobotScene::setEditModeEnabled(bool e)
{
    editModeEnabled = e;

    if (e)
    {
        oldWallPositions.clear();
        for (QHash<QGraphicsItem *, bool>::iterator it=walls.begin();
             it!=walls.end(); ++it)
        {
            if (!it.value())
            {
                oldWallPositions[it.key()] = it.key()->pos();

                CResizablePixmapGraphicsItem *r =
                        static_cast<CResizablePixmapGraphicsItem *>(it.key());
                r->setMovable(true);
                r->setResizable(true);
            }
        }

        update();
    }
    else
    {
        for (QHash<QGraphicsItem *, bool>::iterator it=walls.begin();
             it!=walls.end(); ++it)
        {
            if (!it.value())
            {
                CResizablePixmapGraphicsItem *r =
                        static_cast<CResizablePixmapGraphicsItem *>(it.key());
                r->setMovable(false);
                r->setResizable(false);

                // New wall or position of existing changed?
                if (!oldWallPositions.contains(r) ||
                    (r->pos() != oldWallPositions[r]))
                    lightingDirty = true;
            }
        }

        if (lightingDirty)
        {
            updateLighting(); // calls update() too
            lightingDirty = false;
        }
        else
            update();

        dragging = false;
    }
}

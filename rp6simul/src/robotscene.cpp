#include "lightgraphicsitem.h"
#include "progressdialog.h"
#include "robotscene.h"
#include "resizablepixmapgraphicsitem.h"
#include "robotgraphicsitem.h"
#include "rp6simul.h"
#include "utils.h"

#include <QtGui>

namespace {

qreal getTransformScaleWidth(const QTransform &tr)
{
    return tr.mapRect(QRectF(0, 0, 1.0, 1.0)).width();
}

QRectF getMinRect(QRectF r, qreal minw, qreal minh)
{
    if (r.width() < minw)
        r.setWidth(minw);
    if (r.height() < minh)
        r.setHeight(minh);
    return r;
}

QPolygonF createShadowPolygon(const QPointF &light, float rad,
                              const QPointF &vertex1, const QPointF &vertex2)
{
    /* General shadow shape is as fallows:

                      -|
                   -    |
                -        |
             -    (2)     |
          |                |
     (1)  |                 | (3)
          |                |
             -    (4)     |
                -        |
                   -    |
                      -|


     With (1) being the obstacle. All lines (2-4) have
     a sufficient long length to reach much beyound the
     lighting area to make sure everything is darkened.
     This length is just 'brute forced' and hopefully
     should be long enough
    */

    // To make sure everything is (hopefully) completely darkened
    const qreal longdist = rad * 3.0;

    /* line (2)
     Start from light center towards the first vertex to
     get line with right direction. Then extrapolate
     towards end of light circle and use vertex and
     end-point of line to construct line (2)
    */

    QLineF line2(light, vertex1);
    // Extrapolate to end of light circle
    line2.setLength(longdist);
    line2.setP1(vertex1);

    /* Line (4)
      We need line (2) and (4) to make (3)
    */
    QLineF line4(light, vertex2);
    line4.setLength(longdist);
    line4.setP1(vertex2);

    /* line (3)
     This is a line from the light center towards the middle of
     the end points from lines (2) and (4), which is then again
     extrapolated beyound the light area
    */

    const QLineF tmpline(line2.p2(), line4.p2());
    QLineF line3(light, tmpline.pointAt(0.5));
    line3.setLength(longdist);

    // Construct polygon
    QPolygonF ret;
    ret << line2.p1() << line2.p2() <<
           line3.p2() <<
           line4.p2() << line4.p1();
    return ret;
}


}


QList<QGraphicsItem *> CRobotScene::fixedSizeItems;

CRobotScene::CRobotScene(QObject *parent) :
    QGraphicsScene(parent),
    backGroundPixmap(QPixmap(getResourcePath("floor.jpg")).scaled(300, 300,
                                                                  Qt::IgnoreAspectRatio,
                                                                  Qt::SmoothTransformation)),
    lightingDirty(false), ambientLight(0.35), shadowQuality(SH_QUALITY_MED),
    blockPixmap(QPixmap(getResourcePath("wall.jpg")).scaled(30, 30,
                                                            Qt::IgnoreAspectRatio,
                                                            Qt::SmoothTransformation)),
    boxPixmap(getResourcePath("cardboard-box.png")), running(false),
    robotAdvanceDelay(1000.0 / 33.0), followRobot(false), viewAngle(0.0),
    dragging(false), draggedEnough(false), mouseMode(MODE_POINT),
    lightEditMode(false), mapEdited(false), gridSize(15.0),
    autoGridEnabled(true), gridVisible(false)
{
    qRegisterMetaTypeStreamOperators<SLightSettings>("SLightSettings");
    qRegisterMetaTypeStreamOperators<SObjectSettings>("SObjectSettings");

    setBackgroundBrush(Qt::darkGray);

    robotGraphicsItem = new CRobotGraphicsItem;
    addItem(robotGraphicsItem);
    robotGraphicsItem->setVisible(false);
    connect(robotGraphicsItem, SIGNAL(robotMoved(const QPointF &, qreal)),
            this, SLOT(updateRobotFollowing()));

    QPixmap pm = QPixmap(getResourcePath("robot-start.png"))
            .scaledToHeight(robotGraphicsItem->boundingRect().height(),
                            Qt::SmoothTransformation);
    robotStartGraphicsItem = new CRotatablePixmapGraphicsItem(pm);
    robotStartGraphicsItem->setDeletable(false);
    addItem(robotStartGraphicsItem);
    connect(robotStartGraphicsItem, SIGNAL(posChanged(const QPointF &)), this,
            SLOT(markMapEdited()));
    connect(robotStartGraphicsItem, SIGNAL(rotationChanged(qreal)), this,
            SLOT(markMapEdited()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(advance()));
    timer->start(robotAdvanceDelay);
}

QGraphicsView *CRobotScene::getGraphicsView() const
{
    QGraphicsView *ret = views()[0];
    Q_ASSERT(ret);
    return ret;
}

void CRobotScene::scaleGraphicsView(qreal f)
{
    QGraphicsView *view = getGraphicsView();

    const qreal factor = getTransformScaleWidth(view->transform().scale(f, f));
    if (factor > 20.0)
        return;

    if ((f < 1.0) && (!view->horizontalScrollBar()->isVisible() &&
        !view->verticalScrollBar()->isVisible()))
        return;

    view->scale(f, f);

    // Keep the size fixed of certain items (ie handles).
    // HACK: only do this when zooming out: scaling handles slightly gives
    // them an offset, which is more noticable for zooming in.
    if (factor <= 1.0)
    {
        foreach (QGraphicsItem *it, fixedSizeItems)
            it->setTransform(QTransform().scale(1.0/f, 1.0/f), true);
    }
}

void CRobotScene::rotateView(qreal angle)
{
    QGraphicsView *view = getGraphicsView();
    QTransform tr(view->transform());
    const QPointF c(sceneRect().center());
    tr.translate(c.x(), c.y());
    tr.rotate(viewAngle - angle);
    tr.translate(-c.x(), -c.y());
    view->setTransform(tr);
    viewAngle = angle;
}

QRectF CRobotScene::getDragRect(bool squared) const
{
    QRectF ret;
    QPointF p(mouseDragStartPos);
    if (autoGridEnabled)
        p = alignPosToGrid(p);
    ret.setTopLeft(p);

    p = QPointF(mousePos);
    if (autoGridEnabled)
        p = alignPosToGrid(p);
    ret.setBottomRight(p);

    if (squared)
    {
        qreal aw = qAbs(ret.width()), ah = qAbs(ret.height());
        if (aw > ah)
        {
            if (ret.height() < 0.0)
                ret.setHeight(-aw);
            else
                ret.setHeight(aw);
        }
        else
        {
            if (ret.width() < 0.0)
                ret.setWidth(-ah);
            else
                ret.setWidth(ah);
        }
    }

    return ret.normalized();
}

void CRobotScene::handleDirtyLighting()
{
    if (!lightEditMode)
        updateLighting();
    else
        lightingDirty = true;
}

void CRobotScene::updateMouseCursor()
{
    QWidget *vp = getGraphicsView()->viewport();
    Q_ASSERT(vp);

    if (mouseMode == MODE_POINT)
        vp->unsetCursor();
    else
        vp->setCursor(Qt::CrossCursor);
}

void CRobotScene::updateMapSize()
{
    qDebug() << "Update map size";
    updateStaticWalls();
    markMapEdited(true);

    for (QList<SLight>::iterator it=lights.begin(); it!=lights.end(); ++it)
        it->dirty = true;
    handleDirtyLighting();
}

QGraphicsRectItem *CRobotScene::createStaticWall() const
{
    QGraphicsRectItem *ret = new QGraphicsRectItem;
    ret->setPen(Qt::NoPen);
    ret->setBrush(blockPixmap);
    return ret;
}

void CRobotScene::updateStaticWalls()
{
    const QRectF sr(sceneRect());

    if (staticWalls.isEmpty())
    {
        addItem(staticWalls[WALL_LEFT] = createStaticWall());
        addItem(staticWalls[WALL_RIGHT] = createStaticWall());
        addItem(staticWalls[WALL_TOP] = createStaticWall());
        addItem(staticWalls[WALL_BOTTOM] = createStaticWall());
    }

    const qreal blocksize = 30.0;

    staticWalls[WALL_LEFT]->setPos(sr.x(), sr.y());
    staticWalls[WALL_LEFT]->setRect(0.0, 0.0, blocksize, sr.height());

    staticWalls[WALL_RIGHT]->setPos(sr.right()-blocksize, sr.y());
    staticWalls[WALL_RIGHT]->setRect(0.0, 0.0, blocksize, sr.height());

    staticWalls[WALL_TOP]->setPos(sr.x()+blocksize, sr.y());
    staticWalls[WALL_TOP]->setRect(0.0, 0.0, sr.width()-(2.0*blocksize),
                                   blocksize);

    staticWalls[WALL_BOTTOM]->setPos(sr.x()+blocksize, sr.bottom()-blocksize);
    staticWalls[WALL_BOTTOM]->setRect(0.0, 0.0, sr.width()-(2.0*blocksize),
                                      blocksize);
}

void CRobotScene::addLight(const QPointF &p, float r)
{
    CLightGraphicsItem *l = new CLightGraphicsItem(r);
    l->setPos(p - QPointF(r, r));
    l->setVisible(lightEditMode);
    addItem(l);
    connect(l, SIGNAL(removed(CBaseGraphicsItem *)), this,
            SLOT(removeLight(CBaseGraphicsItem *)));
    connect(l, SIGNAL(posChanged(const QPointF &)), this,
            SLOT(markMapEdited()));
    connect(l, SIGNAL(posChanged(const QPointF &)), this,
            SLOT(markLightingDirty(const QPointF &)));
    connect(l, SIGNAL(radiusChanged(float)), this, SLOT(markMapEdited()));
    connect(l, SIGNAL(radiusChanged(float)), this,
            SLOT(markLightingDirty(float)));
    lights << SLight(l);
}

void CRobotScene::addWall(const QRectF &rect)
{
    CResizablePixmapGraphicsItem *resi
            = new CResizablePixmapGraphicsItem(blockPixmap, true);
    resi->setPos(rect.topLeft());
    resi->setSize(rect.size());
    addItem(resi);
    connect(resi, SIGNAL(removed(CBaseGraphicsItem *)), this,
            SLOT(removeWall(CBaseGraphicsItem *)));
    connect(resi, SIGNAL(posChanged(const QPointF &)),
            this, SLOT(markMapEdited()));
    connect(resi, SIGNAL(posChanged(const QPointF &)),
            this, SLOT(markLightingDirty(const QPointF &)));
    connect(resi, SIGNAL(sizeChanged(const QSizeF &)), this,
            SLOT(markMapEdited()));
    connect(resi, SIGNAL(sizeChanged(const QSizeF &)), this,
            SLOT(markLightingDirty(const QSizeF &)));
    dynamicWalls << resi;
}

void CRobotScene::addBox(const QRectF &rect)
{
    CResizablePixmapGraphicsItem *resi =
            new CResizablePixmapGraphicsItem(boxPixmap, false);
    resi->setPos(rect.topLeft());
    resi->setSize(rect.size());
    addItem(resi);
    connect(resi, SIGNAL(destroyed(QObject*)), this, SLOT(removeBox(QObject*)));
    connect(resi, SIGNAL(posChanged(const QPointF &)), this,
            SLOT(markMapEdited()));
    boxes << resi;
}

void CRobotScene::removeLight(CBaseGraphicsItem *it)
{
    qDebug() << "Removing light";

    const int size = lights.size();
    for (int i=0; i<size; ++i)
    {
        if (lights[i].item == it)
        {
            lights.removeAt(i);
            break;
        }
    }

    markMapEdited(true);
    handleDirtyLighting();
}

void CRobotScene::removeWall(CBaseGraphicsItem *it)
{
    qDebug() << "Removing wall";

    const QRectF wrect(it->pos(), it->boundingRect().size());
    const int size = lights.size();
    for (int i=0; i<size; ++i)
    {
        if (lights[i].dirty)
            continue;

        const QRectF lightrect(lights[i].item->pos(),
                               lights[i].item->boundingRect().size());
        if (lightrect.intersects(wrect))
        {
            qDebug() << "marking light" << i;
            lights[i].dirty = true;
        }
    }

    dynamicWalls.removeOne(static_cast<CResizablePixmapGraphicsItem *>(it));
    markMapEdited(true);
    handleDirtyLighting();
}

void CRobotScene::removeBox(QObject *o)
{
    qDebug() << "Removing box";
    CResizablePixmapGraphicsItem *b =
            static_cast<CResizablePixmapGraphicsItem *>(o);
    boxes.removeOne(b);
    markMapEdited(true);
}

void CRobotScene::markMapEdited(bool e)
{
    if (e != mapEdited)
    {
        mapEdited = e;
        emit mapEditedChanged(e);
    }
}

void CRobotScene::markLightingDirty(const QPointF &oldp)
{
    CBaseGraphicsItem *it = qobject_cast<CBaseGraphicsItem *>(sender());
    Q_ASSERT(it);

    if (!it)
        return;

    const QRectF orect(oldp, it->boundingRect().size());
    const QRectF nrect(it->pos(), it->boundingRect().size());

    const int size = lights.size();
    for (int i=0; i<size; ++i)
    {
        if (lights[i].dirty)
            continue;

        const QRectF lightrect(lights[i].item->pos(),
                               lights[i].item->boundingRect().size());
        if (lightrect.intersects(orect) || lightrect.intersects(nrect))
        {
            qDebug() << "marking light" << i;
            lights[i].dirty = true;
        }
    }

    handleDirtyLighting();
}

void CRobotScene::markLightingDirty(const QSizeF &olds)
{
    CBaseGraphicsItem *it = qobject_cast<CBaseGraphicsItem *>(sender());
    Q_ASSERT(it);

    if (!it)
        return;

    const QRectF orect(it->pos(), olds);
    const QRectF nrect(it->pos(), it->boundingRect().size());

    const int size = lights.size();
    for (int i=0; i<size; ++i)
    {
        if (lights[i].dirty)
            continue;

        const QRectF lightrect(lights[i].item->pos(),
                               lights[i].item->boundingRect().size());
        if (lightrect.intersects(orect) || lightrect.intersects(nrect))
        {
            qDebug() << "marking light" << i;
            lights[i].dirty = true;
        }
    }

    handleDirtyLighting();
}

void CRobotScene::markLightingDirty(float)
{
    CLightGraphicsItem *it = qobject_cast<CLightGraphicsItem *>(sender());
    Q_ASSERT(it);

    if (!it)
        return;

    const int size = lights.size();
    for (int i=0; i<size; ++i)
    {
        if (lights[i].item == it)
        {
            lights[i].dirty = true;
            break;
        }
    }

    handleDirtyLighting();
}

void CRobotScene::updateRobotFollowing()
{
    if (followRobot)
    {
        rotateView(robotGraphicsItem->rotation());
        getGraphicsView()->centerOn(robotGraphicsItem);
    }
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

    if (!lightEditMode && !lightImage.isNull())
    {
        painter->setCompositionMode(QPainter::CompositionMode_HardLight);
        painter->drawImage(sceneRect().toRect(), lightImage);
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    if (dragging && draggedEnough)
    {
        if (mouseMode == MODE_WALL)
        {
            painter->setPen(Qt::NoPen);
            painter->setOpacity(0.75);
            painter->setBrush(blockPixmap);
            painter->drawRect(getDragRect(false));
        }
        else if (mouseMode == MODE_BOX)
        {
            painter->setPen(Qt::NoPen);
            painter->setOpacity(0.75);
            painter->drawPixmap(getDragRect(false).toRect(), boxPixmap);
        }
        else if (mouseMode == MODE_LIGHT)
        {
            const QRectF rect(getDragRect(true));
            const qreal radius = rect.height()/2.0;
            QRadialGradient g(rect.center(), radius);
            g.setColorAt(0.0, QColor(255, 255, 0, 200));
            g.setColorAt(1.0, QColor(255, 180, 0, 50));
            painter->setBrush(g);
            painter->drawEllipse(rect);
        }
    }

    if (gridVisible)
    {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(Qt::blue, 0.25));

        const float startx = sceneRect().left(), endx = sceneRect().right();
        const float starty = sceneRect().top(), endy = sceneRect().bottom();

        for (float x=startx; x<=endx; x+=gridSize)
            painter->drawLine(x, starty, x, endy);

        for (float y=starty; y<=endy; y+=gridSize)
            painter->drawLine(startx, y, endx, y);
    }

    robotGraphicsItem->drawLEDs(painter);

    painter->restore();
}

void CRobotScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{    
    if ((mouseMode == MODE_POINT) ||
        (event->button() != Qt::LeftButton))
        QGraphicsScene::mousePressEvent(event);
    else if ((mouseMode == MODE_WALL) || (mouseMode == MODE_BOX) ||
             (mouseMode == MODE_LIGHT))
    {
        mouseDragStartPos = event->scenePos();
        dragging = true;
        draggedEnough = false;
    }
}

void CRobotScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    mousePos = event->scenePos();
    if (mouseMode == MODE_POINT)
        QGraphicsScene::mouseMoveEvent(event);
    else if (dragging)
    {
        if (!draggedEnough)
        {
            const QSizeF s(getDragRect(mouseMode == MODE_LIGHT).size());
            const qreal minsize = 7.5;
            draggedEnough = ((s.width() >= minsize) && (s.height() >= minsize));
        }

        if (draggedEnough)
            update();
    }
}

void CRobotScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if ((event->button() == Qt::RightButton))
        setMouseMode(MODE_POINT, true);

    if ((mouseMode == MODE_POINT) ||
        (event->button() != Qt::LeftButton))
        QGraphicsScene::mouseReleaseEvent(event);
    else if (draggedEnough)
    {
        const qreal minsize = 15.0;

        if (mouseMode == MODE_WALL)
        {
            const QRectF r(getMinRect(getDragRect(false), minsize, minsize));
            addWall(r);

            for (QList<SLight>::iterator it=lights.begin();
                 it!=lights.end(); ++it)
            {
                if (it->dirty)
                    continue;
                const QRectF lightrect(it->item->pos(),
                                       it->item->boundingRect().size());
                if (lightrect.intersects(r))
                    it->dirty = true;
            }

            handleDirtyLighting();
            markMapEdited(true);
        }
        else if (mouseMode == MODE_BOX)
        {
            addBox(getMinRect(getDragRect(false), minsize, minsize));
            markMapEdited(true);
        }
        else if (mouseMode == MODE_LIGHT)
        {
            const QRectF r(getMinRect(getDragRect(true), minsize, minsize));
            const float radius = r.height()/2.0;
            addLight(r.center(), radius);
            markMapEdited(true);
            handleDirtyLighting();
        }
    }

    dragging = draggedEnough = false;
}

void CRobotScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    // Based on code from http://www.qtcentre.org/threads/35738-How-to-zooming-like-in-AutoCAD-with-QGraphicsView

    // zoom only when CTRL key pressed
    if (event->modifiers().testFlag(Qt::ControlModifier))
    {
        const int numsteps = event->delta() / 15 / 8;

        if (numsteps == 0)
        {
            event->ignore();
            return;
        }

        scaleGraphicsView(pow(1.1, numsteps));

        if (!followRobot && (numsteps > 0))
        {
            QGraphicsView *view = getGraphicsView();
            view->centerOn(event->scenePos());
        }

        event->accept();
    }
}

void CRobotScene::start()
{
    robotStartGraphicsItem->setVisible(false);
    robotGraphicsItem->setPos(robotStartGraphicsItem->pos());
    robotGraphicsItem->setRotation(robotStartGraphicsItem->rotation());
    robotGraphicsItem->setVisible(true);
    robotGraphicsItem->start();
    running = true;
}

void CRobotScene::stop()
{
    robotStartGraphicsItem->setVisible(true);
    robotGraphicsItem->setVisible(false);
    robotGraphicsItem->stop();
    running = false;
}

void CRobotScene::setMouseMode(EMouseMode mode, bool sign)
{
    mouseMode = mode;
    if (mode != MODE_POINT)
    {
        QList<QGraphicsItem *> sitems(selectedItems());
        foreach (QGraphicsItem *it, sitems)
            it->setSelected(false);
        setFollowRobot(false, true);
    }

    update();

    if (sign)
        emit mouseModeChanged(mode);

    updateMouseCursor();
}

void CRobotScene::setMapSize(const QSizeF &size, bool force)
{
    const QRectF newsrect(0.0, 0.0, size.width(), size.height());

    if (!force)
    {
        // Check for any items outside the new rect and move them to center
        // of the map
        bool asked = false;
        const QList<QGraphicsRectItem *> swalls(staticWalls.values());
        foreach (QGraphicsItem *it, items())
        {
            if (it->parentItem())
                continue; // Skip child items

            QGraphicsRectItem *rit = qgraphicsitem_cast<QGraphicsRectItem *>(it);
            if (rit && swalls.contains(rit))
                continue; // Skip static walls

            QRectF r(it->mapRectToScene(it->boundingRect()));
            if (!newsrect.contains(r)) // Not fully contained?
            {
                if (!asked)
                {
                    if (QMessageBox::question(CRP6Simulator::getInstance(),
                                              "Items outside new map size",
                                              "One or more items are outside the new map size.\n"
                                              "If you proceed these will be moved to the new center.",
                                              QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
                        return;
                    asked = true;
                }

                r.moveCenter(newsrect.center());
                it->setPos(r.topLeft());
            }

        }
    }

    setSceneRect(newsrect);
    updateMapSize();
}

void CRobotScene::setAmbientLight(float l)
{
    if (!qFuzzyCompare(l, ambientLight))
    {
        ambientLight = l;
        markMapEdited(true);

        for (QList<SLight>::iterator it=lights.begin(); it!=lights.end(); ++it)
            it->dirty = true;
        handleDirtyLighting();
    }
}

void CRobotScene::setShadowQuality(EShadowQuality q)
{
    if (q != shadowQuality)
    {
        shadowQuality = q;
        markMapEdited(true);

        for (QList<SLight>::iterator it=lights.begin(); it!=lights.end(); ++it)
            it->dirty = true;
        handleDirtyLighting();
    }
}

void CRobotScene::setGridSize(float s)
 {
    if (!qFuzzyCompare(s, gridSize))
    {
        gridSize = s;
        update();
        markMapEdited(true);
    }
}

void CRobotScene::setAutoGrid(bool a)
{
    if (a != autoGridEnabled)
    {
        autoGridEnabled = a;
        markMapEdited(true);
    }
}

QPointF CRobotScene::alignPosToGrid(QPointF pos) const
{
    int rx = (int)pos.x() % (int)gridSize, ry = (int)pos.y() % (int)gridSize;

    if (rx >= gridSize/2.0)
        pos.rx() += (gridSize - rx);
    else
        pos.rx() -= rx;

    if (ry >= gridSize/2.0)
        pos.ry() += (gridSize - ry);
    else
        pos.ry() -= ry;

    return pos;
}

void CRobotScene::newMap(QSettings &settings, const QSizeF &size)
{
    // NOTE: Only values without proper default are written

    settings.beginGroup("map");

    settings.setValue("version", 1);
    settings.setValue("mapSize", size);
    settings.setValue("ambientLight", 0.75);

    settings.endGroup();
}

void CRobotScene::saveMap(QSettings &settings)
{
    /* Settings:
        - Version nr*
        - Map size*
        - Grid size*
        - Auto snap*
        - Lights*
        - Auto refresh lighting*
        - Ambient light*
        - Dynamic walls*
        - Robot start position*
        - Scale*
        - Boxes*
    */

    QList<QVariant> lightsvarlist;
    foreach (const SLight &l, lights)
    {
        QVariant v;
        v.setValue(SLightSettings(l.item->pos(), l.item->getRadius()));
        lightsvarlist << v;
    }

    QList<QVariant> wallsvarlist;
    foreach (CResizablePixmapGraphicsItem *w, dynamicWalls)
    {
        QVariant v;
        v.setValue(SObjectSettings(w->pos(), w->getSize()));
        wallsvarlist << v;
    }

    QList<QVariant> boxesvarlist;
    foreach (CResizablePixmapGraphicsItem *w, boxes)
    {
        QVariant v;
        v.setValue(SObjectSettings(w->pos(), w->getSize()));
        boxesvarlist << v;
    }

    settings.beginGroup("map");

    settings.setValue("version", 1);
    settings.setValue("mapSize", sceneRect().size());
    settings.setValue("gridSize", gridSize);
    settings.setValue("autoGrid", autoGridEnabled);
    settings.setValue("lights", lightsvarlist);
    settings.setValue("ambientLight", ambientLight);
    settings.setValue("shadowQuality", shadowQuality);
    settings.setValue("walls", wallsvarlist);
    settings.setValue("boxes", boxesvarlist);
    settings.setValue("robotStartPosition", robotStartGraphicsItem->pos());
    settings.setValue("robotStartRotation", robotStartGraphicsItem->rotation());
    settings.setValue("scale", getTransformScaleWidth(getGraphicsView()->transform()));

    settings.endGroup();

    settings.sync(); // force commit

    markMapEdited(false);
}

void CRobotScene::loadMap(QSettings &settings)
{
    clearMap();

    settings.beginGroup("map");

    if (settings.value("version", -1) != 1)
        QMessageBox::warning(CRP6Simulator::getInstance(), "Map version",
                             "Missing/unsupported map version");

    // First set grid size: setting map size may trigger grid update
    gridSize = settings.value("gridSize", 15.0).toReal();

    const QSizeF mapsize(settings.value("mapSize",
                                        QSizeF(1000.0, 1000.0)).toSizeF());

    if (mapsize != sceneRect().size())
        setMapSize(mapsize, true);

    autoGridEnabled = settings.value("autoGrid", true).toBool();

    QList<QVariant> varlist = settings.value("lights").toList();
    foreach (QVariant v, varlist)
    {
        SLightSettings s(v.value<SLightSettings>());
        addLight(s.pos + QPointF(s.radius, s.radius), s.radius);
    }

    ambientLight = settings.value("ambientLight", 0.35).toFloat();
    shadowQuality =
            static_cast<EShadowQuality>(settings.value("shadowQuality", 1).toInt());

    varlist = settings.value("walls").toList();
    foreach (QVariant v, varlist)
    {
        SObjectSettings s(v.value<SObjectSettings>());
        addWall(QRectF(s.pos, s.size));
    }

    varlist = settings.value("boxes").toList();
    foreach (QVariant v, varlist)
    {
        SObjectSettings s(v.value<SObjectSettings>());
        addBox(QRectF(s.pos, s.size));
    }

    QPointF startpos(settings.value("robotStartPosition",
                                    sceneRect().center()).toPointF());
    qreal startrot = settings.value("robotStartRotation", 0.0).toReal();
    robotStartGraphicsItem->setPos(startpos);
    robotStartGraphicsItem->setRotation(startrot);
    if (running)
    {
        robotGraphicsItem->setPos(startpos);
        robotGraphicsItem->setRotation(startrot);
    }

    const qreal scale = settings.value("scale", 1.0).toReal();
    const qreal curscale = getTransformScaleWidth(getGraphicsView()->transform());
    scaleGraphicsView(scale / curscale);

    settings.endGroup();
    updateLighting();
    markMapEdited(false);
    getGraphicsView()->centerOn(robotStartGraphicsItem);
}

float CRobotScene::getIntensity(QPointF point) const
{
    if (lightImage.isNull())
        return 0;

    const int maxc = 90; // Keep this in sync with intensityToColor()!

    const qreal scalew = (qreal)lightImage.width() / sceneRect().width();
    const qreal scaleh = (qreal)lightImage.height() / sceneRect().height();

    point.rx() *= scalew;
    point.ry() *= scaleh;

    // Although we take the blue color, we could have taken any of them
    const int intensity = qBlue(lightImage.pixel(point.toPoint()));
    return qMin((float)intensity / (float)maxc, 2.0f);
}

void CRobotScene::zoomSceneIn()
{
    scaleGraphicsView(1.2);
}

void CRobotScene::zoomSceneOut()
{
    scaleGraphicsView(1/1.2);
}

void CRobotScene::setFollowRobot(bool f, bool sign)
{
     followRobot = f;
     if (!f)
         rotateView(0.0);
     else
     {
         updateRobotFollowing(); // Trigger initial focus in case robot doesn't move
         if (mouseMode != MODE_POINT)
             setMouseMode(MODE_POINT, true);
     }

     if (sign)
         emit robotFollowingChanged(f);
}

void CRobotScene::updateLighting()
{
    /*
      This function creates a pixmap that is used for lighting. Several
      composiing (blending) modes are used to graphically present lighting/
      darking effects. The resulting lighting image is kept at a lower scale
      to enhance performance. A small side effect is that at especially
      high scale factors shadows around walls appear at a small offset.

      For each light an image is produced (and cached) which contain the
      lighting gradient. On each of these images shadows are casted using
      polygons created with createShadowPolygon(). All the light images
      are blended ("Plus compositing") to produce the final lighting pixmap.
      By doing so multiple overlapping light sources are supported.

      The lighting gradients and shadow polygons may overlap any walls,
      therefore as a final step all the walls are drawn over to restore
      normal lighting on each of them.
    */

    QElapsedTimer startt;

    startt.start();

    qreal scale;

    switch (shadowQuality)
    {
    case SH_QUALITY_LOW: scale = 6.0; break;
    default:
    case SH_QUALITY_MED: scale = 3.0; break;
    case SH_QUALITY_HIGH: scale = 1.0; break;
    }

    QTransform lighttrans;
    lighttrans.scale(1.0/scale, 1.0/scale);

    QList<QPolygonF> obstacles;

    foreach (QGraphicsItem *w, staticWalls.values())
        obstacles << w->mapToScene(w->boundingRect());

    foreach (QGraphicsItem *w, dynamicWalls)
        obstacles << w->mapToScene(w->boundingRect());

    // Loop through all lights and generate the shadow polygons. Note that
    // these polygons are not scaled down yet
    const int lsize = lights.size();
    for (int i=0; i<lsize; ++i)
    {
        if (!lights[i].dirty)
            continue;

        lights[i].dirty = false;

        const qreal rad = lights[i].item->boundingRect().width() / 2.0;
        QList<QPolygonF> shadowPolys;
        const QRectF lightrect(lights[i].item->pos(),
                               lights[i].item->boundingRect().size());

        bool darkened = false;
        foreach (QPolygonF ob, obstacles)
        {
            QRectF obr(ob.boundingRect());
            if (lightrect.intersects(obr))
            {
                if (ob.containsPoint(lightrect.center(), Qt::OddEvenFill))
                {
                    // Light center is within an obstacle, therefore no
                    // light can be produced.
                    darkened = true;
                    break;
                }

                // Create obstacle polygon that is positioned relative
                // to this (unscaled) light image.
                QPolygonF obtr(ob.translated(-lights[i].item->pos()));
                obtr.pop_back(); // Remove double start/end point

                /*
                 For each polygon obstacle, get the two vertices that
                 should be used for shadow casting. When the light beam
                 towards a vertice is seen as horizontal axis, all the
                 other vertices should either fall above or below this
                 point.
                */
                QList<QPointF> shadowverts;
                int i = 0, j = 0;
                foreach (QPointF v, obtr)
                {
                    QLineF line(QPointF(rad, rad), v);
                    QTransform tr;
                    tr.translate(v.x(), v.y());
                    tr.rotate(line.angle());
                    tr.translate(-v.x(), -v.y());

                    QPolygonF p(tr.map(obtr));

                    bool hasup = false, hasdown = false;

                    foreach (QPointF pv, p)
                    {
                        if (i == j)
                        {
                            ++j;
                            continue;
                        }

                        if (qFuzzyCompare(pv.y(), v.y()))
                            continue;

                        hasup = (hasup || (pv.y() > v.y()));
                        hasdown = (hasdown || (pv.y() < v.y()));

                        if (hasup && hasdown)
                            break;

                        ++j;
                    }

                    if ((hasup && !hasdown) || (!hasup && hasdown))
                        shadowverts << v;

                    ++i;
                }

                Q_ASSERT(shadowverts.size() == 2);

                if (shadowverts.size() == 2)
                {
                    shadowPolys << createShadowPolygon(QPointF(rad, rad),
                                                       rad, shadowverts[0],
                                                       shadowverts[1]);
                }
            }
        }

        bool upgradient = false;
        const QSize imsize((lights[i].item->boundingRect().size() / scale).toSize());
        if (lights[i].image.isNull() || (lights[i].image.size() != imsize))
        {
            lights[i].image = QImage(imsize, QImage::Format_RGB32);
            lights[i].image.fill(qRgb(0, 0, 0));
            upgradient = true;
        }

        QPainter painter(&lights[i].image);
        painter.setPen(Qt::NoPen);

        if (darkened)
        {
            painter.fillRect(0.0, 0.0, imsize.width(), imsize.height(),
                             Qt::black);
            continue;
        }

        if (upgradient)
        {
            // Create light gradient and cache it
            lights[i].gradientImage = QImage(imsize, QImage::Format_RGB32);
            lights[i].gradientImage.fill(qRgb(0, 0, 0));
            QPainter gradp(&lights[i].gradientImage);
            const qreal r = imsize.width() / 2.0;
            QRadialGradient rg(QPointF(r, r), r);
            rg.setColorAt(0.0, intensityToColor(2.0 - ambientLight));
            rg.setColorAt(1.0, intensityToColor(0.0));
            gradp.setBrush(rg);
            gradp.drawEllipse(0, 0, r*2.0, r*2.0);
        }

        painter.drawImage(0, 0, lights[i].gradientImage);
        painter.drawRect(QRectF(QPointF(0.0, 0.0), imsize));

        painter.setBrush(Qt::black);
        painter.setRenderHint(QPainter::Antialiasing);
        foreach (QPolygonF p, shadowPolys)
            painter.drawPolygon(lighttrans.map(p));

        painter.end();
    }

    const QSize lisize(sceneRect().size().toSize() / scale);
    if (lightImage.isNull() || (lightImage.size() != lisize))
        lightImage = QImage(lisize, QImage::Format_RGB32);

    lightImage.fill(intensityToColor(ambientLight).rgb());

    QPainter lipainter(&lightImage);
    lipainter.setRenderHint(QPainter::Antialiasing);
    lipainter.setPen(Qt::NoPen);

    lipainter.setCompositionMode(QPainter::CompositionMode_Plus);
    foreach (const SLight &l, lights)
        lipainter.drawImage(l.item->pos() / scale, l.image);

    lipainter.setCompositionMode(QPainter::CompositionMode_SourceOut);
    lipainter.setBrush(QColor(127, 127, 127));
    foreach (QPolygonF ob, obstacles)
        lipainter.drawPolygon(lighttrans.map(ob));

    lightingDirty = false;
    update();

    qDebug() << "Updated lightingMap in" << startt.elapsed() << "ms.";
}

void CRobotScene::clearMap()
{
    const QList<QGraphicsRectItem *> swalls(staticWalls.values());
    foreach (QGraphicsItem *it, items())
    {
        if (it->parentItem())
            continue; // Don't remove child items

        QGraphicsRectItem *rit = qgraphicsitem_cast<QGraphicsRectItem *>(it);
        if (rit && swalls.contains(rit))
            continue; // Skip static walls

        if ((it == robotGraphicsItem) || (it == robotStartGraphicsItem))
            continue;

        delete it;
    }

    markMapEdited(true);

    boxes.clear();
    dynamicWalls.clear();
    lights.clear();

    handleDirtyLighting();
}

void CRobotScene::setLightEditMode(bool v)
{
    if (v == lightEditMode)
        return;

    lightEditMode = v;
    foreach (const SLight &l, lights)
        l.item->setVisible(v);

    if (!v)
    {
        if (mouseMode == MODE_LIGHT)
            setMouseMode(MODE_POINT, true);

        if (lightingDirty)
            updateLighting();
    }

    update();
}


QColor intensityToColor(float intensity)
{
    intensity = qBound(0.0f, intensity, 2.0f);
    const float maxc = 90.0;
    const int c = qRound(intensity * maxc);
    return QColor(c, c, c);
}

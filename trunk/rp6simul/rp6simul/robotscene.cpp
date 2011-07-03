#include "lightgraphicsitem.h"
#include "progressdialog.h"
#include "robotscene.h"
#include "resizablepixmapgraphicsitem.h"
#include "robotgraphicsitem.h"
#include "rp6simul.h"

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

}

CRobotScene::CRobotScene(QObject *parent) :
    QGraphicsScene(parent),
    backGroundPixmap(QPixmap("../resource/floor.jpg").scaled(300, 300,
                                                             Qt::IgnoreAspectRatio,
                                                             Qt::SmoothTransformation)),
    lightingDirty(false), autoRefreshLighting(true),
    lightItemsVisible(false), ambientLight(0.35),
    blockPixmap(QPixmap("../resource/wall.jpg").scaled(30, 30,
                                                       Qt::IgnoreAspectRatio,
                                                       Qt::SmoothTransformation)),
    boxPixmap("../resource/cardboard-box.png"), followRobot(false),
    viewAngle(0.0), dragging(false), draggedEnough(false),
    mouseMode(MODE_POINT), editModeEnabled(false), mapEdited(false),
    gridSize(15.0), autoGridEnabled(true), gridVisible(false)
{
    setBackgroundBrush(Qt::darkGray);

    robotGraphicsItem = new CRobotGraphicsItem;
    robotGraphicsItem->setPos(50, 450); // UNDONE
    addItem(robotGraphicsItem);
    connect(robotGraphicsItem, SIGNAL(posChanged(const QPointF &)), this,
            SLOT(robotPosChanged()));

    qRegisterMetaTypeStreamOperators<SLightSettings>("SLightSettings");
    qRegisterMetaTypeStreamOperators<SObjectSettings>("SObjectSettings");
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

QRectF CRobotScene::getDragRect() const
{
    QRectF ret;
    ret.setTopLeft(mouseDragStartPos);
    ret.setBottomRight(mousePos);
    return ret.normalized();
}

QRectF CRobotScene::getLightDragRect(void) const
{
    QRectF ret;
    ret.setTopLeft(mouseDragStartPos);
    ret.setBottom(mousePos.y());
    ret.setWidth(ret.height());
    return ret.normalized();
}

void CRobotScene::updateMouseCursor()
{
    QWidget *vp = getGraphicsView()->viewport();
    Q_ASSERT(vp);

    if (!editModeEnabled || (mouseMode == MODE_POINT))
        vp->unsetCursor();
    else
        vp->setCursor(Qt::CrossCursor);
}

void CRobotScene::updateMapSize()
{
    qDebug() << "Update map size";
    updateGrid();
    updateStaticWalls();
    markMapEdited(true);
    lightingDirty = true;
}

void CRobotScene::updateGrid()
{
    gridImage = QImage(sceneRect().size().toSize(),
                       QImage::Format_ARGB32_Premultiplied);
    gridImage.fill(Qt::transparent);
    QPainter painter(&gridImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::blue, 0.25));

    const float startx = sceneRect().left(), endx = sceneRect().right();
    const float starty = sceneRect().top(), endy = sceneRect().bottom();

    for (float x=startx; x<=endx; x+=gridSize)
        painter.drawLine(x, starty, x, endy);

    for (float y=starty; y<=endy; y+=gridSize)
        painter.drawLine(startx, y, endx, y);
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
    l->setVisible(editModeEnabled);
    addItem(l);
    connect(l, SIGNAL(destroyed(QObject*)), this, SLOT(removeLight(QObject*)));
    connect(l, SIGNAL(posChanged(const QPointF &)), this, SLOT(markMapEdited()));
    connect(l, SIGNAL(posChanged(const QPointF &)), this,
            SLOT(markLightingDirty(const QPointF &)));
    connect(l, SIGNAL(radiusChanged()), this, SLOT(markMapEdited()));
    connect(l, SIGNAL(radiusChanged()), this, SLOT(markLightingDirty()));    
    lights << SLight(l);
}

void CRobotScene::addWall(const QRectF &rect)
{
    CResizablePixmapGraphicsItem *resi
            = new CResizablePixmapGraphicsItem(blockPixmap, true);
    resi->setPos(rect.topLeft());
    resi->setSize(rect.size());
    addItem(resi);
    connect(resi, SIGNAL(destroyed(QObject*)), this,
            SLOT(removeWall(QObject*)));
    connect(resi, SIGNAL(posChanged(const QPointF &)),
            this, SLOT(markMapEdited()));
    connect(resi, SIGNAL(posChanged(const QPointF &)),
            this, SLOT(markLightingDirty(const QPointF &)));
    connect(resi, SIGNAL(sizeChanged()), this, SLOT(markMapEdited()));
    connect(resi, SIGNAL(sizeChanged()), this, SLOT(markLightingDirty()));
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
    connect(resi, SIGNAL(posChanged()), this, SLOT(markMapEdited()));
    boxes << resi;
}

void CRobotScene::updateItemsEditMode()
{
    foreach (const SLight &l, lights)
        l.item->setVisible(editModeEnabled && lightItemsVisible);

    foreach (CResizablePixmapGraphicsItem *w, dynamicWalls)
    {
        w->setMovable(editModeEnabled);
        w->setResizable(editModeEnabled);
        w->setDeletable(editModeEnabled);
    }
}

void CRobotScene::removeLight(QObject *o)
{
    qDebug() << "Removing light";
    CLightGraphicsItem *l = static_cast<CLightGraphicsItem *>(o);

    const int size = lights.size();
    for (int i=0; i<size; ++i)
    {
        if (lights[i].item == l)
        {
            lights.removeAt(i);
            break;
        }
    }

    markMapEdited(true);
    lightingDirty = true;
}

void CRobotScene::removeWall(QObject *o)
{
    qDebug() << "Removing wall";
    CResizablePixmapGraphicsItem *w =
            static_cast<CResizablePixmapGraphicsItem *>(o);
    dynamicWalls.removeOne(w);
    markMapEdited(true);
    lightingDirty = true;
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

    lightingDirty = true;

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

    updateLighting();
}

void CRobotScene::robotPosChanged()
{
    if (followRobot && !editModeEnabled)
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

//    if (!editModeEnabled)
    {
        painter->setCompositionMode(QPainter::CompositionMode_HardLight);
        painter->drawImage(0, 0, lightImage);
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    if (dragging && draggedEnough)
    {
        if (mouseMode == MODE_WALL)
        {
            painter->setPen(Qt::NoPen);
            painter->setOpacity(0.75);
            painter->setBrush(blockPixmap);
            painter->drawRect(getDragRect());
        }
        else if (mouseMode == MODE_BOX)
        {
            painter->setPen(Qt::NoPen);
            painter->setOpacity(0.75);
            painter->drawPixmap(getDragRect().toRect(), boxPixmap);
        }
        else if (mouseMode == MODE_LIGHT)
        {
            const QRectF rect(getLightDragRect());
            const qreal radius = rect.height()/2.0;
            QRadialGradient g(rect.center(), radius);
            g.setColorAt(0.0, QColor(255, 255, 0, 200));
            g.setColorAt(1.0, QColor(255, 180, 0, 50));
            painter->setBrush(g);
            painter->drawEllipse(rect);
        }
    }

    if (editModeEnabled && gridVisible)
        painter->drawImage(0, 0, gridImage);

    painter->restore();
}

void CRobotScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{    
    if (!editModeEnabled || (mouseMode == MODE_POINT) ||
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
    if (!editModeEnabled || (mouseMode == MODE_POINT))
        QGraphicsScene::mouseMoveEvent(event);
    else if (dragging)
    {
        if (!draggedEnough)
        {
            const QSizeF s(getDragRect().size());
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

    if (!editModeEnabled || (mouseMode == MODE_POINT) ||
        (event->button() != Qt::LeftButton))
        QGraphicsScene::mouseReleaseEvent(event);
    else if (draggedEnough)
    {
        const qreal minsize = 15.0;

        if (mouseMode == MODE_WALL)
        {
            addWall(getMinRect(getDragRect(), minsize, minsize));
            markMapEdited(true);
            lightingDirty = true;
        }
        else if (mouseMode == MODE_BOX)
        {
            addBox(getMinRect(getDragRect(), minsize, minsize));
            markMapEdited(true);
        }
        else if (mouseMode == MODE_LIGHT)
        {
            const QRectF r(getMinRect(getLightDragRect(), minsize, minsize));
            const float radius = r.height()/2.0;
            addLight(r.center(), radius);
            markMapEdited(true);
            lightingDirty = true;
        }
    }

    dragging = draggedEnough = false;
}

void CRobotScene::setMouseMode(EMouseMode mode, bool sign)
{
    mouseMode = mode;
    if (mode != MODE_POINT)
    {
        QList<QGraphicsItem *> sitems(selectedItems());
        foreach (QGraphicsItem *it, sitems)
            it->setSelected(false);
    }

    update();

    if (sign)
        emit mouseModeChanged(mode);

    updateMouseCursor();
}

void CRobotScene::setEditModeEnabled(bool e)
{
    if (e == editModeEnabled)
        return;

    editModeEnabled = e;

    if (e)
    {
        rotateView(0.0);
        setMouseMode(mouseMode); // Update mode, calls update() too
    }
    else
    {
        if (lightingDirty && autoRefreshLighting)
            updateLighting(); // calls update() too
        else
            update();

        dragging = false;
        updateMouseCursor();
    }

    updateItemsEditMode();
}

void CRobotScene::setMapSize(const QSizeF &size)
{
    setSceneRect(QRectF(0.0, 0.0, size.width(), size.height()));
    updateMapSize();
}

void CRobotScene::setAutoRefreshLighting(bool a)
{
    if (a != autoRefreshLighting)
    {
        autoRefreshLighting = a;
        markMapEdited(true);
    }
}

void CRobotScene::setAmbientLight(float l)
{
    if (!qFuzzyCompare(l, ambientLight))
    {
        ambientLight = l;
        markMapEdited(true);
        lightingDirty = true;
    }
}

void CRobotScene::setGridSize(float s)
 {
    if (!qFuzzyCompare(s, gridSize))
    {
        gridSize = s;
        updateGrid();
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

void CRobotScene::saveMap(QSettings &settings)
{
    /* Settings:
        - Version nr*
        - Map size*
        - Grid visible?
        - Grid size*
        - Auto snap*
        - Lights*
        - Auto refresh lighting*
        - Ambient light*
        - Lights visible (edit mode)?
        - Dynamic walls*
        - Robot start position
        - Scale*
        - Boxes*
        - Light/shadow pixmaps*
        - Screenshot
    */

    // UNDONE: Update lightmap if necessary?
    // Or only save if outside edit mode?

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
    settings.setValue("autoRefreshLighting", autoRefreshLighting);
    settings.setValue("ambientLight", ambientLight);
    settings.setValue("walls", wallsvarlist);
    settings.setValue("boxes", boxesvarlist);
    settings.setValue("scale", getTransformScaleWidth(getGraphicsView()->transform()));

    QElapsedTimer etimer;
    etimer.start();

    // UNDONE!

    settings.sync();
//    settings.setValue("shadowImage", shadowImage);
    settings.sync();
    qDebug() << "Wrote shadow image after" << etimer.elapsed() << "ms";
    settings.setValue("lightImage", lightImage);
    settings.sync();
    qDebug() << "Wrote light image after" << etimer.elapsed() << "ms";

    settings.endGroup();

    // UNDONE: Should be here? (ie map export)
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
        setMapSize(mapsize); // Triggers updateGrid
    else
        updateGrid();

    autoGridEnabled = settings.value("autoGrid", true).toBool();

    QList<QVariant> varlist = settings.value("lights").toList();
    foreach (QVariant v, varlist)
    {
        SLightSettings s(v.value<SLightSettings>());
        addLight(s.pos + QPointF(s.radius, s.radius), s.radius);
    }

    autoRefreshLighting = settings.value("autoRefreshLighting", true).toBool();
    ambientLight = settings.value("ambientLight", 0.35).toFloat();

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

    const qreal scale = settings.value("scale", 1.0).toReal();
    const qreal curscale = getTransformScaleWidth(getGraphicsView()->transform());
    scaleGraphicsView(scale / curscale);

    lightImage = settings.value("lightImage").value<QImage>();

    if (lightImage.isNull() ||
        (lightImage.size() != sceneRect().size().toSize()))
    {
        qWarning("Failed to load light maps. Regenerating...");
        updateLighting();
        markMapEdited(true);
    }
    else
    {
        markMapEdited(false);
        lightingDirty = false;
        update();
    }

    settings.endGroup();

    updateItemsEditMode();
}

void CRobotScene::zoomSceneIn()
{
    scaleGraphicsView(1.2);
}

void CRobotScene::zoomSceneOut()
{
    scaleGraphicsView(1/1.2);
}

void CRobotScene::setFollowRobot(bool f)
{
     followRobot = f;
     if (!f)
         rotateView(0.0);
     else
         robotPosChanged(); // Trigger initial focus in case robot doesn't move
}

void CRobotScene::updateLighting()
{
    QTime startt;
    startt.start();

    CProgressDialog prgdialog(true, CRP6Simulator::getInstance());
    prgdialog.setWindowTitle("Updating light map...");
    prgdialog.setRange(0, 100);
    prgdialog.setValue(33);
    prgdialog.setLabelText("Determining lighting regions...");
//    prgdialog.show();

    QList<QPolygonF> obstacles;

    foreach (QGraphicsItem *w, staticWalls.values())
        obstacles << w->mapToScene(w->boundingRect());

    foreach (QGraphicsItem *w, dynamicWalls)
        obstacles << w->mapToScene(w->boundingRect());

//    qApp->processEvents();
    if (!prgdialog.wasCanceled())
    {
        prgdialog.setLabelText("Performing per pixel lighting...");
        prgdialog.setValue(66);

        const int lsize = lights.size();
        for (int i=0; i<lsize; ++i)
        {
            if (prgdialog.wasCanceled())
                break;

            if (!lights[i].dirty)
                continue;

            lights[i].dirty = false;

            const QSize imsize(lights[i].item->boundingRect().size().toSize());
            if (lights[i].image.isNull() || (lights[i].image.size() != imsize))
            {
                lights[i].image = QImage(imsize, QImage::Format_RGB32);
                lights[i].image.fill(qRgb(0, 0, 0));
            }

            QPainter painter(&lights[i].image);
            painter.setPen(Qt::NoPen);

            const qreal rad = lights[i].item->boundingRect().width() / 2.0;
            QRadialGradient rg(QPointF(rad, rad), rad);

            float intensity =
                    lights[i].item->intensityAt(lights[i].item->pos() + QPointF(rad, rad),
                                                obstacles);
            intensity = qMin(intensity, 2.0f);
            int c = qRound(intensity * 127.0 * 0.65);
            rg.setColorAt(0.0, QColor(c, c, c));

            intensity = lights[i].item->intensityAt(lights[i].item->pos(), obstacles);
            intensity = qMin(intensity, 2.0f);
            c = qRound(intensity * 127.0 * 0.65);
            rg.setColorAt(1.0, QColor(c, c, c));

            QList<QPolygonF> shadowPolys;

            const QRectF lightrect(lights[i].item->pos(),
                                   lights[i].item->boundingRect().size());

            foreach (QPolygonF ob, obstacles)
            {
                QRectF obr(ob.boundingRect());
                if (lightrect.intersects(obr))
                {
                    if (ob.containsPoint(lightrect.center(), Qt::OddEvenFill))
                        continue;

                    QPolygonF obtr(ob.translated(-lights[i].item->pos()));
                    obtr.pop_back(); // Remove double start/end point

                    QList<QPointF> ambverts;
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
                            if (qFuzzyCompare(pv.y(), v.y()))
                                continue;
                            hasup = (hasup || (pv.y() > v.y()));
                            hasdown = (hasdown || (pv.y() < v.y()));

                            if (hasup && hasdown)
                                break;
                        }

                        if ((hasup && !hasdown) || (!hasup && hasdown))
                            ambverts << v;
                    }

                    Q_ASSERT(ambverts.size() == 2);

                    if (ambverts.size() == 2)
                    {
#if 0
                        /* General shadow shape is as fallows:

                                        (3)
                                   - - - - - -|
                                -             |
                             -    (2)         |
                          |                   |
                     (1)  |                   |   (4)
                          |                   |
                             -    (6)         |
                                -             |
                                   - - - - - -|
                                        (5)

                         With (1) being the obstacle, (2,6) lines towards
                         end of light radius, (3,4,5) straight lines to
                         completely cut the shadow out of the light image.
                        */

                        const QPointF rellpos(rad, rad);

                        /* line (2)
                         Start from light center towards the first vertex to
                         get line with right direction. Then extrapolate
                         towards end of light circle and use vertex and
                         end-point of line to construct line (2)
                        */

                        QLineF line2(rellpos, ambverts[0]);
                        // Extrapolate to end of light circle
                        line2.setLength(rad);
                        line2.setP1(ambverts[0]);

                        /* line (3)
                         Here a line is constructed with an angle depending
                         of the angle from line (2): it is in such a way
                         chosen that a straight horizontal/vertical line is
                         obtained.
                        */

                        QLineF line3(line2.p2(), rellpos);
                        qreal targetangle = ((int)line2.angle() / 90) * 90.0;
                        line3.setLength(rad);
                        line3.setAngle(targetangle);
                        qDebug() << "line3:" << line3 << targetangle;

                        /* Line (4)
                          No need to construct it: automatically follows from
                          polygon made below.
                        */

                        /* Line (6)
                          We need this line first before line (5) can be
                          constructed
                        */
                        QLineF line6(rellpos, ambverts[1]);
                        line6.setLength(rad);
                        line6.setP1(ambverts[1]);

                        /* Line (5)
                        */
                        QLineF line5(line6.p2(), rellpos);
                        targetangle = ((int)line6.angle() / 90) * 90.0;
                        line5.setLength(rad);
                        line5.setAngle(targetangle);
                        qDebug() << "line5:" << line5 << targetangle;

                        // Construct polygon
                        QPolygonF p;
                        p << line2.p1() << line2.p2() <<
                             line3.p2() <<
                             line5.p2() << line5.p1() <<
                             line6.p1();
                        shadowPolys << p;
#endif
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

                        const QPointF rellpos(rad, rad);
                        // to make sure everything is completely darkened
                        const qreal longdist = rad * 3.0;

                        /* line (2)
                         Start from light center towards the first vertex to
                         get line with right direction. Then extrapolate
                         towards end of light circle and use vertex and
                         end-point of line to construct line (2)
                        */

                        QLineF line2(rellpos, ambverts[0]);
                        // Extrapolate to end of light circle
                        line2.setLength(longdist);
                        line2.setP1(ambverts[0]);

                        /* line (3)
                         This is a line from the light center
                         towards the object center, which is then again
                         extrapolated beyound the light area
                        */

                        QLineF line3(rellpos, obtr.boundingRect().center());
                        line3.setLength(longdist);

                        /* Line (4)
                        */
                        QLineF line4(rellpos, ambverts[1]);
                        line4.setLength(longdist);
                        line4.setP1(ambverts[1]);

                        // Construct polygon
                        QPolygonF p;
                        p << line2.p1() << line2.p2() <<
                             line3.p2() <<
                             line4.p2() << line4.p1();
                        shadowPolys << p;
                    }
                }
            }

            painter.setBrush(rg);
            painter.drawEllipse(lights[i].item->boundingRect());

            painter.setBrush(Qt::black);
            painter.setRenderHint(QPainter::Antialiasing);
            foreach (QPolygonF p, shadowPolys)
                painter.drawPolygon(p);

            painter.end();
        }

        if (lightImage.isNull() ||
            (lightImage.size() != sceneRect().size().toSize()))
            lightImage = QImage(sceneRect().size().toSize(),
                                QImage::Format_RGB32);

        const int ambc = qRound(ambientLight * 127.0 * 0.65);
        lightImage.fill(qRgb(ambc, ambc, ambc));

        QPainter lipainter(&lightImage);
        lipainter.setRenderHint(QPainter::Antialiasing);
        lipainter.setPen(Qt::NoPen);

        lipainter.setCompositionMode(QPainter::CompositionMode_Plus);
        foreach (const SLight &l, lights)
            lipainter.drawImage(l.item->pos(), l.image);

        /*mpainter.setCompositionMode(QPainter::CompositionMode_ColorDodge);
        mpainter.fillRect(0, 0, mergedimg.width(), mergedimg.height(),
                          QColor(50, 50, 50));*/

        lipainter.setCompositionMode(QPainter::CompositionMode_SourceOut);
        lipainter.setBrush(QColor(127, 127, 127));
        foreach (QPolygonF ob, obstacles)
            lipainter.drawPolygon(ob);
    }  

    lightingDirty = false;
    markMapEdited(true);
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
            continue;

        if (it == robotGraphicsItem)
            continue;

        CResizablePixmapGraphicsItem *reit =
                dynamic_cast<CResizablePixmapGraphicsItem *>(it);
        if (reit)
        {
            // Remove wall/box if present
            dynamicWalls.removeOne(reit);
            boxes.removeOne(reit);
        }

        delete it;
    }

    markMapEdited(true);
    lightingDirty = (lightingDirty || !lights.isEmpty());
    lights.clear();
}

void CRobotScene::setLightItemsVisible(bool v)
{
    lightItemsVisible = v;
    if (editModeEnabled)
    {
        foreach (const SLight &l, lights)
            l.item->setVisible(v);
    }
}

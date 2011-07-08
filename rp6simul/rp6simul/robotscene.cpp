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

QColor intensityToColor(float intensity)
{
    intensity = qBound(0.0f, intensity, 2.0f);
    const float maxc = 90;
    const int c = qRound(intensity * maxc);
    return QColor(c, c, c);
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

void CRobotScene::handleDirtyLighting()
{
    if (autoRefreshLighting)
        updateLighting();
    else
        lightingDirty = true;
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

    for (QList<SLight>::iterator it=lights.begin(); it!=lights.end(); ++it)
        it->dirty = true;
    handleDirtyLighting();
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

    // UNDONE
    if (/*!editModeEnabled*/!lightPixmap.isNull())
    {
        painter->setCompositionMode(QPainter::CompositionMode_HardLight);
        painter->drawPixmap(sceneRect().toRect(), lightPixmap);
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
            handleDirtyLighting();
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
            handleDirtyLighting();
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
        if (lightingDirty && a)
            updateLighting();
    }
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
    settings.setValue("lightImage", lightPixmap);
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

    // UNDONE!
//    lightImage = settings.value("lightImage").value<QImage>();

    if (lightPixmap.isNull() ||
        (lightPixmap.size() != sceneRect().size().toSize()))
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

    const qreal scale = 5.0; // UNDONE: Configurable?
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
                        shadowverts << v;
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
        if (lights[i].image.isNull())
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
    if (lightPixmap.isNull() || (lightPixmap.size() != lisize))
        lightPixmap = QPixmap(lisize);

    lightPixmap.fill(intensityToColor(ambientLight).rgb());

    QPainter lipainter(&lightPixmap);
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
            continue; // Skip static walls

        if (it == robotGraphicsItem)
            continue;

        delete it;
    }

    markMapEdited(true);

    boxes.clear();
    dynamicWalls.clear();
    lights.clear();

    handleDirtyLighting();
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

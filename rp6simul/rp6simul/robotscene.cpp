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
    connect(robotGraphicsItem, SIGNAL(posChanged()), this,
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

    staticWalls[WALL_LEFT]->setPos(sr.x(), sr.y());
    staticWalls[WALL_LEFT]->setRect(0.0, 0.0, 30.0, sr.height());

    staticWalls[WALL_RIGHT]->setPos(sr.right()-30.0, sr.y());
    staticWalls[WALL_RIGHT]->setRect(0.0, 0.0, 30.0, sr.height());

    staticWalls[WALL_TOP]->setPos(sr.x(), sr.y());
    staticWalls[WALL_TOP]->setRect(0.0, 0.0, sr.width(), 30.0);

    staticWalls[WALL_BOTTOM]->setPos(sr.x(), sr.bottom()-30.0);
    staticWalls[WALL_BOTTOM]->setRect(0.0, 0.0, sr.width(), 30.0);
}

void CRobotScene::addLight(const QPointF &p, float r)
{
    CLightGraphicsItem *l = new CLightGraphicsItem(r);
    l->setPos(p - QPointF(r, r));
    l->setVisible(editModeEnabled);
    addItem(l);
    connect(l, SIGNAL(destroyed(QObject*)), this, SLOT(removeLight(QObject*)));
    connect(l, SIGNAL(posChanged()), this, SLOT(markMapEdited()));
    connect(l, SIGNAL(posChanged()), this, SLOT(markLightingDirty()));
    connect(l, SIGNAL(radiusChanged()), this, SLOT(markLightingDirty()));
    lights << l;
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
    connect(resi, SIGNAL(posChanged()), this, SLOT(markMapEdited()));
    connect(resi, SIGNAL(posChanged()), this, SLOT(markLightingDirty()));
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
    foreach (CLightGraphicsItem *l, lights)
        l->setVisible(editModeEnabled && lightItemsVisible);

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
    lights.removeOne(l);
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

    if (!editModeEnabled)
    {
        painter->drawImage(0, 0, shadowImage);
//        painter->drawImage(sceneRect(), shadowImage);

        painter->setCompositionMode(QPainter::CompositionMode_Overlay);
        painter->drawImage(0, 0, lightImage);
//        painter->drawImage(sceneRect(), lightImage);
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
    foreach (CLightGraphicsItem *l, lights)
    {
        QVariant v;
        v.setValue(SLightSettings(l->pos(), l->getRadius()));
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

    settings.sync();
    settings.setValue("shadowImage", shadowImage);
    settings.sync();
    qDebug() << "Wrote shadow image after" << etimer.elapsed() << "ms";
    settings.setValue("lightImage", lightImage);
    settings.sync();
    qDebug() << "Wrote shadow image after" << etimer.elapsed() << "ms";

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

    shadowImage = settings.value("shadowImage").value<QImage>();
    lightImage = settings.value("lightImage").value<QImage>();

    if (shadowImage.isNull() || lightImage.isNull() ||
        (shadowImage.size() != sceneRect().size().toSize()) ||
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
    prgdialog.show();

    prgdialog.setLabelText("Creating image caches...");
    prgdialog.setValue(0);

    shadowImage = QImage(sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    shadowImage.fill(qAlpha(0));
    QPainter shpainter(&shadowImage);
    shpainter.setRenderHint(QPainter::Antialiasing);

    prgdialog.setValue(16);
    qApp->processEvents();

    lightImage = QImage(sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    lightImage.fill(qRgb(127, 127, 127));
    QPainter lipainter(&lightImage);
    lipainter.setRenderHint(QPainter::Antialiasing);

    qDebug() << "Elapsed" << startt.elapsed() << "ms";

    QRegion lightregion;
    QRegion shadowregion(sceneRect().toAlignedRect());
    QList<QPolygonF> obstacles;

    qApp->processEvents();
    if (!prgdialog.wasCanceled())
    {
        prgdialog.setLabelText("Determining lighting regions...");
        prgdialog.setValue(33);

        foreach (CLightGraphicsItem *l, lights)
            lightregion += l->mapToScene(l->boundingRect()).boundingRect().toAlignedRect();

        foreach (QGraphicsItem *w, staticWalls.values())
        {
            obstacles << w->mapToScene(w->boundingRect());
            const QRect r(w->mapToScene(w->boundingRect()).boundingRect().toAlignedRect());
            lightregion -= r;
            shadowregion -= r;
        }

        foreach (QGraphicsItem *w, dynamicWalls)
        {
            obstacles << w->mapToScene(w->boundingRect());
            const QRect r(w->mapToScene(w->boundingRect()).boundingRect().toAlignedRect());
            lightregion -= r;
            shadowregion -= r;
        }

        shadowregion -= lightregion;

        if (ambientLight > 1.0)
        {
            lipainter.setPen(Qt::NoPen);
            const int c = ambientLight * 127 / 100;
            lipainter.setBrush(QColor(c, c, c));
            lipainter.drawRects(shadowregion.rects());
        }
        else if (ambientLight < 1.0)
        {
            const int alpha = 255 - qRound(ambientLight * 255.0);
            shpainter.setBrush(QColor(0, 0, 0, alpha));
            shpainter.setPen(Qt::NoPen);
            shpainter.drawRects(shadowregion.rects());
        }
    }

    qApp->processEvents();
    if (!prgdialog.wasCanceled())
    {
        prgdialog.setLabelText("Performing per pixel lighting...");
        prgdialog.setValue(66);

        const QVector<QRect> lightrects = lightregion.rects();
        int totw = 0, progw = 0, upcounter = 0;

        foreach (QRect r, lightrects)
            totw += r.width();

        qDebug() << "Starting per pixel after" << startt.elapsed() << "ms";

        foreach (QRect lrect, lightrects)
        {
            if (prgdialog.wasCanceled())
                break;

            int left = lrect.left(), right = lrect.right();
            int top = lrect.top(), bottom = lrect.bottom();

            if (left < 0)
                left = 0;
            if (top < 0)
                top = 0;

            for (int x=left; x<=right; ++x)
            {
                if (prgdialog.wasCanceled())
                    break;

                ++progw;

                for (int y=top; y<=bottom; ++y)
                {
                    ++upcounter;

                    if (upcounter == 50) // UNDONE
                    {
    //                    qDebug() << "prog refresh after " << startt.elapsed() << "ms";
                        prgdialog.setValue(66 + (progw * 100 / totw) / 3);
                        qApp->processEvents();
                        if (prgdialog.wasCanceled())
                            break;
                        upcounter = 0;
                    }

                    float intensity = ambientLight;
                    const QPointF p(x, y);
                    foreach (CLightGraphicsItem *l, lights)
                    {
                        intensity += l->intensityAt(p, obstacles);
                        if (intensity >= 2.0)
                            break;
                    }

                    if (intensity > 1.0)
                    {
                        intensity = qMin(intensity, 2.0f);
                        const int c = qRound(intensity * 127.0);
                        lipainter.setPen(QColor(c, c, c));
                        lipainter.drawPoint(x, y);
                    }
                    else if (intensity < 1.0)
                    {
                        const int alpha = 255 - qRound(intensity * 255.0);
                        shpainter.setPen(QPen(QBrush(QColor(0, 0, 0, alpha)), 1.0));
                        shpainter.drawPoint(x, y);
                    }
                }
            }
        }
    }
/*
    shpainter.end();
    lipainter.end();

    shadowImage = shadowImage.scaled(shadowImage.size() * 0.5,
                                     Qt::IgnoreAspectRatio,
                                     Qt::SmoothTransformation);
    lightImage = lightImage.scaled(lightImage.size() * 0.5,
                                   Qt::IgnoreAspectRatio,
                                   Qt::SmoothTransformation);*/


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
        foreach (CLightGraphicsItem *l, lights)
            l->setVisible(v);
    }
}

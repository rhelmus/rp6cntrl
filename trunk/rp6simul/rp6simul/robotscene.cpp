#include "lightgraphicsitem.h"
#include "robotscene.h"
#include "resizablepixmapgraphicsitem.h"
#include "robotgraphicsitem.h"
#include "rp6simul.h"

#include <QtGui>

namespace {

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
    boxPixmap("../resource/cardboard-box.png"),
    dragging(false), draggedEnough(false), mouseMode(MODE_POINT),
    editModeEnabled(false), gridSize(15.0), autoGridEnabled(true),
    gridVisible(false), viewAngle(0.0), followRobot(false)
{
    setBackgroundBrush(Qt::darkGray);

    robotGraphicsItem = new CRobotGraphicsItem;
    robotGraphicsItem->setPos(50, 450); // UNDONE
    addItem(robotGraphicsItem);
    connect(robotGraphicsItem, SIGNAL(posChanged()), this,
            SLOT(robotPosChanged()));

    connect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(updateMapSize()));
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

    // Based on Qt's elastic nodes example
    qreal factor = view->transform().scale(f, f).mapRect(QRectF(0, 0, 1.0, 1.0)).width();
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

void CRobotScene::updateMapSize()
{
    updateGrid();
    updateStaticWalls();
    lightingDirty = true;
}

void CRobotScene::removeLight(QObject *o)
{
    qDebug() << "Removing light";
    CLightGraphicsItem *l = static_cast<CLightGraphicsItem *>(o);
    lights.removeOne(l);
    lightingDirty = true;
}

void CRobotScene::removeWall(QObject *o)
{
    qDebug() << "Removing wall";
    CResizablePixmapGraphicsItem *w = static_cast<CResizablePixmapGraphicsItem *>(o);
    dynamicWalls.removeOne(w);
    lightingDirty = true;
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

        painter->setCompositionMode(QPainter::CompositionMode_Overlay);
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
            lightingDirty = true;
        }
        else if (mouseMode == MODE_BOX)
        {
            const QRectF r(getMinRect(getDragRect(), minsize, minsize));
            CResizablePixmapGraphicsItem *resi =
                    new CResizablePixmapGraphicsItem(boxPixmap, false);
            resi->setPos(r.topLeft());
            resi->setSize(r.size());
            addItem(resi);
        }
        else if (mouseMode == MODE_LIGHT)
        {
            const QRectF r(getMinRect(getLightDragRect(), minsize, minsize));
            const float radius = r.height()/2.0;
            addLight(r.center(), radius);
            lightingDirty = true;
        }
    }

    dragging = draggedEnough = false;
}

void CRobotScene::addLight(const QPointF &p, float r)
{
    CLightGraphicsItem *l = new CLightGraphicsItem(r);
    l->setPos(p - QPointF(r, r));
    l->setVisible(editModeEnabled);
    addItem(l);
    connect(l, SIGNAL(destroyed(QObject*)), this, SLOT(removeLight(QObject*)));
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
    connect(resi, SIGNAL(posChanged()), this, SLOT(markLightingDirty()));
    connect(resi, SIGNAL(sizeChanged()), this, SLOT(markLightingDirty()));
    dynamicWalls << resi;
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
        if (lightItemsVisible)
        {
            foreach (CLightGraphicsItem *l, lights)
                l->setVisible(true);
        }

        foreach (CResizablePixmapGraphicsItem *w, dynamicWalls)
        {
            w->setMovable(true);
            w->setResizable(true);
            w->setDeletable(true);
        }

        rotateView(0.0);
        setMouseMode(mouseMode); // Update mode, calls update() too
    }
    else
    {
        foreach (CLightGraphicsItem *l, lights)
            l->setVisible(false);

        foreach (CResizablePixmapGraphicsItem *w, dynamicWalls)
        {
            w->setMovable(false);
            w->setResizable(false);
            w->setDeletable(false);
        }

        if (lightingDirty && autoRefreshLighting)
            updateLighting(); // calls update() too
        else
            update();

        dragging = false;
        updateMouseCursor();
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

    shadowImage = QImage(sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    shadowImage.fill(Qt::transparent);
    QPainter shpainter(&shadowImage);
    shpainter.setRenderHint(QPainter::Antialiasing);

    lightImage = QImage(sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    lightImage.fill(Qt::transparent);
    QPainter lipainter(&lightImage);
    lipainter.setRenderHint(QPainter::Antialiasing);

    QProgressDialog prgdialog(CRP6Simulator::getInstance(),
                              Qt::FramelessWindowHint);
    prgdialog.setLabelText("Calculating light map...");
    prgdialog.setRange(0, sceneRect().width());
    prgdialog.setMinimumDuration(50);
    prgdialog.setValue(0);
    prgdialog.setCancelButton(0); // No canceling

    QList<QPolygonF> obstacles;
    foreach (QGraphicsItem *w, staticWalls.values())
        obstacles << w->mapToScene(w->boundingRect());
    foreach (QGraphicsItem *w, dynamicWalls)
        obstacles << w->mapToScene(w->boundingRect());

    const int left = sceneRect().left(), right = sceneRect().right();
    const int top = sceneRect().top(), bottom = sceneRect().bottom();
    for (int x=left; x<=right; ++x)
    {
        if (!(x % 30))
        {
            prgdialog.setValue(x);
            qApp->processEvents();
        }

        for (int y=top; y<=bottom; ++y)
        {
            float intensity = ambientLight;
            const QPointF p(x, y);
            bool inwall = false;

            foreach (QPolygonF obpoly, obstacles)
            {
                // Pixel in wall?
                if (obpoly.containsPoint(p, Qt::OddEvenFill))
                {
                    intensity = 0.6;
                    inwall = true;
                    break;
                }
            }

            if (!inwall)
            {
                foreach (CLightGraphicsItem *l, lights)
                {
                    intensity += l->intensityAt(p, obstacles);
                    if (intensity >= 2.0)
                        break;
                }
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

    lightingDirty = false;
    update();

    qDebug() << "Updated lightingMap in" << startt.elapsed() << "ms.";
}

void CRobotScene::clearMap()
{
    const QList<QGraphicsRectItem *> swalls(staticWalls.values());
    foreach (QGraphicsItem *it, items())
    {
        QGraphicsRectItem *rit = qgraphicsitem_cast<QGraphicsRectItem *>(it);
        if (rit && swalls.contains(rit))
            continue;

        if (it == robotGraphicsItem)
            continue;

        CResizablePixmapGraphicsItem *reit =
                dynamic_cast<CResizablePixmapGraphicsItem *>(it);
        if (reit && dynamicWalls.contains(reit))
            dynamicWalls.removeOne(reit);

        delete it;
    }

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

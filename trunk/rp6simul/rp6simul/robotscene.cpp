#include "lightgraphicsitem.h"
#include "robotscene.h"
#include "resizablepixmapgraphicsitem.h"
#include "robotgraphicsitem.h"
#include "rp6simul.h"

#include <QtGui>

CRobotScene::CRobotScene(QObject *parent) :
    QGraphicsScene(parent), blockSize(30.0, 30.0),
    blockPixmap(QPixmap("../resource/wall.jpg").scaled(blockSize.toSize(),
                                                       Qt::IgnoreAspectRatio,
                                                       Qt::SmoothTransformation)),
    boxPixmap("../resource/cardboard-box.png"),
    backGroundPixmap(QPixmap("../resource/floor.jpg").scaled(300, 300,
                                                             Qt::IgnoreAspectRatio,
                                                             Qt::SmoothTransformation)),
    lightingDirty(false), dragging(false), mouseMode(MODE_POINT),
    editModeEnabled(false)
{
    setBackgroundBrush(Qt::darkGray);

    robotGraphicsItem = new CRobotGraphicsItem;
    robotGraphicsItem->setPos(50, 450); // UNDONE
    addItem(robotGraphicsItem);
}

QRectF CRobotScene::getDragRect() const
{
    QRectF ret;
    ret.setTopLeft(mouseDragStartPos);
    ret.setBottomRight(mousePos);
    return ret;
}

QRectF CRobotScene::getLightDragRect(void) const
{
    QRectF ret;
    ret.setTopLeft(mouseDragStartPos);
    ret.setBottom(mousePos.y());
    ret.setWidth(ret.height());
    return ret;
}

void CRobotScene::removeLight(QObject *o)
{
    qDebug() << "Removing light";
    CLightGraphicsItem *l = static_cast<CLightGraphicsItem *>(o);
    lights.removeOne(l);
    markLightingDirty();
}

void CRobotScene::removeWall(QObject *o)
{
    qDebug() << "Removing wall";
    CResizablePixmapGraphicsItem *w = static_cast<CResizablePixmapGraphicsItem *>(o);
    walls.remove(w);
    markLightingDirty();
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
    else if (mouseMode == MODE_LIGHT)
    {
        const QRectF rect(getLightDragRect());
        const float radius = rect.height()/2.0;
        addLight(rect.center(), radius);
        lightingDirty = true;
    }

    dragging = false;
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

void CRobotScene::addWall(const QRectF &rect, bool st)
{
    if (!st)
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
            float intensity = baseintensity;

            foreach (CLightGraphicsItem *l, lights)
            {
                intensity += l->intensityAt(QPointF(x, y), obstacles);
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

void CRobotScene::setMouseMode(EMouseMode mode)
{
    mouseMode = mode;
    if (mode != MODE_POINT)
    {
        QList<QGraphicsItem *> sitems(selectedItems());
        foreach (QGraphicsItem *it, sitems)
            it->setSelected(false);
    }

    update();
}

void CRobotScene::setEditModeEnabled(bool e)
{
    if (e == editModeEnabled)
        return;

    editModeEnabled = e;

    if (e)
    {
        foreach (CLightGraphicsItem *l, lights)
            l->setVisible(true);

        for (QHash<QGraphicsItem *, bool>::iterator it=walls.begin();
             it!=walls.end(); ++it)
        {
            if (!it.value())
            {
                CResizablePixmapGraphicsItem *r =
                        static_cast<CResizablePixmapGraphicsItem *>(it.key());
                r->setMovable(true);
                r->setResizable(true);
            }
        }

        setMouseMode(mouseMode); // Update mode, calls update() too
    }
    else
    {
        foreach (CLightGraphicsItem *l, lights)
            l->setVisible(false);

        for (QHash<QGraphicsItem *, bool>::iterator it=walls.begin();
             it!=walls.end(); ++it)
        {
            if (!it.value())
            {
                CResizablePixmapGraphicsItem *r =
                        static_cast<CResizablePixmapGraphicsItem *>(it.key());
                r->setMovable(false);
                r->setResizable(false);
            }
        }

        if (lightingDirty)
            updateLighting(); // calls update() too
        else
            update();

        dragging = false;
    }

    lightingDirty = false;
}

void CRobotScene::clearMap()
{
    foreach (QGraphicsItem *it, items())
    {
        if (walls.contains(it))
        {
            if (walls[it])
                continue; // Only remove non-static walls
            walls.remove(it);
        }

        if (it == robotGraphicsItem)
            continue;

        delete it;
    }

    const bool uplights = !lights.isEmpty();
    lights.clear();
    if (uplights)
        updateLighting();
}

#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>
#include <QHash>
#include <QMetaType>

class QSettings;

class CLightGraphicsItem;
class CResizablePixmapGraphicsItem;
class CRobotGraphicsItem;

class CRobotScene : public QGraphicsScene
{
public:
    enum EMouseMode { MODE_POINT, MODE_WALL, MODE_BOX, MODE_LIGHT, MODE_DELETE };

private:
    Q_OBJECT

    enum EStaticWall { WALL_LEFT, WALL_RIGHT, WALL_TOP, WALL_BOTTOM };

    QList<CLightGraphicsItem *> lights;
    QPixmap backGroundPixmap;
    QImage shadowImage, lightImage;
    bool lightingDirty, autoRefreshLighting;
    bool lightItemsVisible;
    float ambientLight;
    QHash<EStaticWall, QGraphicsRectItem *> staticWalls;
    QList<CResizablePixmapGraphicsItem *> dynamicWalls;
    QPixmap blockPixmap, boxPixmap;
    QList<CResizablePixmapGraphicsItem *> boxes;
    CRobotGraphicsItem *robotGraphicsItem;
    QPointF mousePos, mouseDragStartPos;    
    bool dragging, draggedEnough;
    EMouseMode mouseMode;
    bool editModeEnabled;
    float gridSize;
    bool autoGridEnabled, gridVisible;
    QImage gridImage;
    qreal viewAngle;
    bool followRobot;

    QGraphicsView *getGraphicsView(void) const;
    void scaleGraphicsView(qreal f);
    void rotateView(qreal angle);
    QRectF getDragRect(void) const;
    QRectF getLightDragRect(void) const;
    void updateMouseCursor(void);
    void updateGrid(void);
    QGraphicsRectItem *createStaticWall(void) const;
    void updateStaticWalls(void);

private slots:
    void updateMapSize(void);
    void removeLight(QObject *o);
    void removeWall(QObject *o);
    void removeBox(QObject *o);
    void markLightingDirty(void) { lightingDirty = true; }
    void robotPosChanged(void);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public:
    explicit CRobotScene(QObject *parent = 0);

    void addLight(const QPointF &p, float r);
    void addWall(const QRectF &rect);
    void addBox(const QRectF &rect);
    void setMouseMode(EMouseMode mode, bool sign=false);
    void setEditModeEnabled(bool e);
    CRobotGraphicsItem *getRobotItem(void) const { return robotGraphicsItem; }
    bool getAutoRefreshLighting(void) const { return autoRefreshLighting; }
    void setAutoRefreshLighting(bool a) { autoRefreshLighting = a; }
    float getAmbientLight(void) const { return ambientLight; }
    void setAmbientLight(float l)
    { if (l != ambientLight) { ambientLight = l; lightingDirty = true; } }
    float getGridSize(void) const { return gridSize; }
    void setGridSize(float s) { gridSize = s; updateGrid(); update(); }
    bool getAutoGrid(void) const { return autoGridEnabled; }
    void setAutoGrid(bool a) { autoGridEnabled = a; }
    QPointF alignPosToGrid(QPointF pos) const;
    void saveMap(QSettings &file);
    void loadMap(QSettings &file);

public slots:
    void zoomSceneIn(void);
    void zoomSceneOut(void);
    void setFollowRobot(bool f);
    void updateLighting(void);
    void clearMap(void);
    void setGridVisible(bool v)
    { if (v != gridVisible) { gridVisible = v; update(); } }
    void setLightItemsVisible(bool v);

signals:
    void mouseModeChanged(CRobotScene::EMouseMode);
};

// Structures for map IO

struct SLightSettings
{
    QPointF pos;
    qreal radius;
    SLightSettings(void) : radius(0.0) { }
    SLightSettings(const QPointF &p, qreal r) : pos(p), radius(r) { }
};

inline QDataStream &operator<<(QDataStream &out, const SLightSettings &ls)
{ out << ls.pos << ls.radius; return out; }
inline QDataStream &operator>>(QDataStream &in, SLightSettings &ls)
{ in >> ls.pos; in >> ls.radius; return in; }

Q_DECLARE_METATYPE(SLightSettings)

struct SObjectSettings
{
    QPointF pos;
    QSizeF size;
    SObjectSettings(void) { }
    SObjectSettings(const QPointF &p, const QSizeF &s) : pos(p), size(s) { }
};

inline QDataStream &operator<<(QDataStream &out, const SObjectSettings &os)
{ out << os.pos << os.size; return out; }
inline QDataStream &operator>>(QDataStream &in, SObjectSettings &os)
{ in >> os.pos; in >> os.size; return in; }

Q_DECLARE_METATYPE(SObjectSettings)

#endif // ROBOTSCENE_H

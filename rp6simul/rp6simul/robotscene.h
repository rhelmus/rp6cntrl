#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>
#include <QHash>
#include <QMetaType>

class CLightGraphicsItem;
class CResizablePixmapGraphicsItem;
class CRobotGraphicsItem;
class QSettings;

class CRobotScene : public QGraphicsScene
{
public:
    enum EMouseMode { MODE_POINT, MODE_WALL, MODE_BOX, MODE_LIGHT, MODE_DELETE };

private:
    Q_OBJECT

    enum EStaticWall { WALL_LEFT, WALL_RIGHT, WALL_TOP, WALL_BOTTOM };

    struct SLightMapRegion
    {
        QImage darkImage, lightImage;
        QPoint pos;
    };

    QList<CLightGraphicsItem *> lights;
    QPixmap backGroundPixmap;
    QImage shadowImage, lightImage;
    QRegion ambientRegion;
    QList<SLightMapRegion> lightMapList;
    bool lightingDirty, autoRefreshLighting;
    bool lightItemsVisible;
    float ambientLight;

    QHash<EStaticWall, QGraphicsRectItem *> staticWalls;
    QList<CResizablePixmapGraphicsItem *> dynamicWalls;
    QPixmap blockPixmap;

    QPixmap boxPixmap;
    QList<CResizablePixmapGraphicsItem *> boxes;

    CRobotGraphicsItem *robotGraphicsItem;
    bool followRobot;
    qreal viewAngle;

    QPointF mousePos, mouseDragStartPos;    
    bool dragging, draggedEnough;
    EMouseMode mouseMode;
    bool editModeEnabled;
    bool mapEdited;

    float gridSize;
    bool autoGridEnabled, gridVisible;
    QImage gridImage;

    QGraphicsView *getGraphicsView(void) const;
    void scaleGraphicsView(qreal f);
    void rotateView(qreal angle);
    QRectF getDragRect(void) const;
    QRectF getLightDragRect(void) const;
    void updateMouseCursor(void);
    void updateMapSize(void);
    void updateGrid(void);
    QGraphicsRectItem *createStaticWall(void) const;
    void updateStaticWalls(void);
    void addLight(const QPointF &p, float r);
    void addWall(const QRectF &rect);
    void addBox(const QRectF &rect);
    void updateItemsEditMode(void);

private slots:
    void removeLight(QObject *o);
    void removeWall(QObject *o);
    void removeBox(QObject *o);
    void markMapEdited(bool e=true);
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

    void setMouseMode(EMouseMode mode, bool sign=false);
    void setEditModeEnabled(bool e);
    CRobotGraphicsItem *getRobotItem(void) const { return robotGraphicsItem; }
    void setMapSize(const QSizeF &size);
    bool getAutoRefreshLighting(void) const { return autoRefreshLighting; }
    void setAutoRefreshLighting(bool a);
    float getAmbientLight(void) const { return ambientLight; }
    void setAmbientLight(float l);
    float getGridSize(void) const { return gridSize; }
    void setGridSize(float s);
    bool getAutoGrid(void) const { return autoGridEnabled; }
    void setAutoGrid(bool a);
    QPointF alignPosToGrid(QPointF pos) const;
    void saveMap(QSettings &settings);
    void loadMap(QSettings &settings);
    bool getMapEdited(void) const { return mapEdited; }

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
    void mapEditedChanged(bool);
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

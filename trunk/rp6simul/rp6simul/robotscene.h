#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>
#include <QHash>
#include <QMetaType>

class CBaseGraphicsItem;
class CLightGraphicsItem;
class CResizablePixmapGraphicsItem;
class CRotatablePixmapGraphicsItem;
class CRobotGraphicsItem;
class QSettings;

class CRobotScene : public QGraphicsScene
{
public:
    enum EMouseMode { MODE_POINT, MODE_WALL, MODE_BOX, MODE_LIGHT };
    enum EShadowQuality { SH_QUALITY_LOW=0, SH_QUALITY_MED, SH_QUALITY_HIGH };

private:
    Q_OBJECT

    enum EStaticWall { WALL_LEFT, WALL_RIGHT, WALL_TOP, WALL_BOTTOM };    

    struct SLight
    {
        CLightGraphicsItem *item;
        QImage image, gradientImage;
        bool dirty;
        SLight(CLightGraphicsItem *it=0) : item(it), dirty(true) { }
    };

    QList<SLight> lights;
    QPixmap backGroundPixmap;
    QImage lightImage;
    bool lightingDirty;
    float ambientLight;
    EShadowQuality shadowQuality;

    QHash<EStaticWall, QGraphicsRectItem *> staticWalls;
    QList<CResizablePixmapGraphicsItem *> dynamicWalls;
    QPixmap blockPixmap;

    QPixmap boxPixmap;
    QList<CResizablePixmapGraphicsItem *> boxes;

    bool running;
    CRotatablePixmapGraphicsItem *robotStartGraphicsItem;
    CRobotGraphicsItem *robotGraphicsItem;
    const float robotAdvanceDelay;
    bool followRobot;
    qreal viewAngle;

    QPointF mousePos, mouseDragStartPos;    
    bool dragging, draggedEnough;
    EMouseMode mouseMode;
    bool lightEditMode;
    bool mapEdited;

    float gridSize;
    bool autoGridEnabled, gridVisible;

    QGraphicsView *getGraphicsView(void) const;
    void scaleGraphicsView(qreal f);
    void rotateView(qreal angle);
    QRectF getDragRect(bool squared) const;
    void handleDirtyLighting(void);
    void updateMouseCursor(void);
    void updateMapSize(void);
    QGraphicsRectItem *createStaticWall(void) const;
    void updateStaticWalls(void);
    void addLight(const QPointF &p, float r);
    void addWall(const QRectF &rect);
    void addBox(const QRectF &rect);

private slots:
    void removeLight(CBaseGraphicsItem *it);
    void removeWall(CBaseGraphicsItem *it);
    void removeBox(QObject *o);
    void markMapEdited(bool e=true);
    void markLightingDirty(const QPointF &oldp);
    void markLightingDirty(const QSizeF &olds);
    void markLightingDirty(float);
    void updateRobotFollowing(void);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent *event);

public:
    explicit CRobotScene(QObject *parent = 0);

    void start(void);
    void stop(void);
    void setMouseMode(EMouseMode mode, bool sign=false);
    CRobotGraphicsItem *getRobotItem(void) const { return robotGraphicsItem; }
    void setMapSize(const QSizeF &size, bool force=false);
    float getAmbientLight(void) const { return ambientLight; }
    void setAmbientLight(float l);
    EShadowQuality getShadowQuality(void) const { return shadowQuality; }
    void setShadowQuality(EShadowQuality q);
    float getRobotAdvanceDelay(void) const { return robotAdvanceDelay; }
    float getGridSize(void) const { return gridSize; }
    void setGridSize(float s);
    bool getAutoGrid(void) const { return autoGridEnabled; }
    void setAutoGrid(bool a);
    QPointF alignPosToGrid(QPointF pos) const;
    void newMap(QSettings &settings, const QSizeF &size);
    void saveMap(QSettings &settings);
    void loadMap(QSettings &settings);
    bool getMapEdited(void) const { return mapEdited; }
    float getIntensity(QPointF point) const;

    static QList<QGraphicsItem *> fixedSizeItems;

public slots:
    void zoomSceneIn(void);
    void zoomSceneOut(void);
    void setFollowRobot(bool f, bool sign=false);
    void updateLighting(void);
    void clearMap(void);
    void setGridVisible(bool v)
    { if (v != gridVisible) { gridVisible = v; update(); } }
    void setLightEditMode(bool v);

signals:
    void mouseModeChanged(CRobotScene::EMouseMode);
    void robotFollowingChanged(bool);
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

QColor intensityToColor(float intensity);

#endif // ROBOTSCENE_H

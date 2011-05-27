#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>
#include <QHash>

class CLightGraphicsItem;
class CRobotGraphicsItem;

class CRobotScene : public QGraphicsScene
{
public:
    enum EMouseMode { MODE_POINT, MODE_WALL, MODE_BOX, MODE_LIGHT, MODE_DELETE };

private:
    Q_OBJECT

    struct SOldLightSettings
    {
        QPointF pos;
        float radius;
        SOldLightSettings(void) : radius(0.0) { }
        SOldLightSettings(const QPointF &p, float r) : pos(p), radius(r) { }
    };

    QList<CLightGraphicsItem *> lights;
    QPixmap backGroundPixmap;
    QImage shadowImage, lightImage;
    bool lightingDirty, lightItemsVisible;
    float ambientLight;
    QHash<QGraphicsItem *, bool> walls;
    QPixmap blockPixmap, boxPixmap;
    CRobotGraphicsItem *robotGraphicsItem;
    QPointF mousePos, mouseDragStartPos;    
    bool dragging, draggedEnough;
    EMouseMode mouseMode;
    bool editModeEnabled;
    float gridSize;
    bool autoGridEnabled, gridVisible;
    QImage gridImage;

    QRectF getDragRect(void) const;
    QRectF getLightDragRect(void) const;
    void updateMouseCursor(void);

private slots:
    void updateGrid(void);
    void removeLight(QObject *o);
    void removeWall(QObject *o);
    void markLightingDirty(void) { if (editModeEnabled) lightingDirty = true; }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public:
    explicit CRobotScene(QObject *parent = 0);

    void addLight(const QPointF &p, float r);
    void addWall(const QRectF &rect, bool st);
    void addWall(qreal x, qreal y, qreal w, qreal h, bool st)
    { addWall(QRectF(x, y, w, h), st); }
    void updateLighting(void);
    void setMouseMode(EMouseMode mode, bool sign=false);
    void setEditModeEnabled(bool e);
    CRobotGraphicsItem *getRobotItem(void) const { return robotGraphicsItem; }
    float getAmbientLight(void) const { return ambientLight; }
    void setAmbientLight(float l) { ambientLight = l; }
    float getGridSize(void) const { return gridSize; }
    void setGridSize(float s) { gridSize = s; updateGrid(); update(); }
    bool getAutoGrid(void) const { return autoGridEnabled; }
    void setAutoGrid(bool a) { autoGridEnabled = a; }
    QPointF alignPosToGrid(QPointF pos) const;

public slots:
    void clearMap(void);
    void setGridVisible(bool v)
    { if (v != gridVisible) { gridVisible = v; update(); } }
    void setLightItemsVisible(bool v);

signals:
    void mouseModeChanged(CRobotScene::EMouseMode);
};

#endif // ROBOTSCENE_H

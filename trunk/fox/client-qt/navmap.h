#ifndef NAVMAP_H
#define NAVMAP_H

#include <QWidget>

class CNavMap: public QWidget
{
    Q_OBJECT

    struct SCell
    {
        QList<QPoint> connections;
        bool isObstacle;
        SCell(void) : isObstacle(false) {}
    };

    QVector<QVector<SCell> > grid;
    QPoint robotPos;
    bool addObstacleMode;
    QPoint currentObstacleMousePos;

    int getCellSize(void) const;
    QRect getCellRect(int x, int y) const;
    QPoint getCellFromPos(const QPoint &pos) const;

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    CNavMap(QWidget *parent = 0, Qt::WindowFlags f = 0);

    void setGrid(const QSize &size);
    void setRobot(const QPoint &pos);
    void markObstacle(const QPoint &pos);
    void connectCells(const QPoint &pos1, const QPoint &pos2);
    QPoint getRobot(void) const { return robotPos; }
    QSize getGridSize(void) const;
    void clearConnections(void);
    bool isObstacle(const QPoint &pos) const;
    bool inGrid(const QPoint &pos) const;

    QSize minimumSizeHint() const { return QSize(250, 250); }
    QSize sizeHint() const { return QSize(400, 400); }

public slots:
    void clearCells(void);
    void toggleObstacleAdd(bool e);
};

#endif // NAVMAP_H

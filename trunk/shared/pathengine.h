#ifndef PATHENGINE_H
#define PATHENGINE_H

#include <set>
#include <stdint.h>

#include <QList>
#include <QMap>
#include <QPoint>
#include <QSize>
#include <QVector>

//#define USE_QTMAP

class CPathEngine
{
public:
    enum EConnection { CONNECTION_LEFT=0, CONNECTION_RIGHT, CONNECTION_UP, CONNECTION_DOWN, MAX_CONNECTIONS };

private:

    struct SConnection
    {
        int xPos, yPos;
        bool broken;
        void set(int x, int y) { xPos = x; yPos = y; broken = false; }
        void breakCon(void) { broken = true; }
        bool connected(int x, int y) const { return (!broken && (x == xPos) && (y == yPos)); }
        SConnection(void) : broken(true) { }
    };

    struct SCell
    {
        SConnection connections[MAX_CONNECTIONS];
        int xPos, yPos;
        uint16_t score;
        int distCost;
        bool open, closed;
        SCell *parent;

        SCell(void) : xPos(0), yPos(0), score(0), distCost(0), open(false), closed(false),
                      parent(NULL) { }
    };

    struct SCellCostCompare // To compare pointers
    {
        bool operator()(SCell *c1, SCell *c2) { return c1->distCost < c2->distCost; }
    };

    typedef QVector<QVector<SCell> > TGrid;
#ifdef USE_QTMAP
    typedef QMultiMap<int, SCell *> TOpenList;
#else
    typedef std::multiset<SCell *, SCellCostCompare> TOpenList;
#endif
    TGrid grid;
    SCell *startCell, *goalCell;
    TOpenList openList;

    void initCell(int x, int y);
    void moveCell(SCell *cell, int dx, int dy);
    QSize getGridSize(void) const;
    int getCellLength(const SCell *c1, const SCell *c2) const;
    EConnection getCellConnection(const SCell *c1, const SCell *c2) const;
    int pathScore(const SCell *c1, const SCell *c2) const;

public:
    CPathEngine(void);

    void setGrid(const QSize &size);
    void expandGrid(int left, int up, int right, int down);
    void initPath(const QPoint &start, const QPoint &goal);
    bool calcPath(QList<QPoint> &output);
    void breakConnection(const QPoint &cell, EConnection connection);
    void breakAllConnections(const QPoint &cell);
};

#endif // PATHENGINE_H

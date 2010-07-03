#ifndef PATHENGINE_H
#define PATHENGINE_H

#include <stdint.h>

#include <QList>
#include <QMap>
#include <QPoint>
#include <QSize>
#include <QVector>

class CPathEngine
{
public:
    enum EConnection { CONNECTION_LEFT=0, CONNECTION_RIGHT, CONNECTION_UP, CONNECTION_DOWN, MAX_CONNECTIONS };

private:

    struct SCell
    {
        SCell *connections[MAX_CONNECTIONS];
        int xPos, yPos;
        uint16_t score;
        int distCost;
        bool open, closed;
        SCell *parent;

        SCell(void) : xPos(0), yPos(0), score(0), distCost(0), open(false), closed(false), parent(NULL)
        { for (int i=0; i<MAX_CONNECTIONS; ++i) connections[i] = NULL;  }
        bool operator<(SCell *other) { return distCost < other->distCost; }
    };

    struct SCellCostCompare
    {
        bool operator()(SCell *c1, SCell *c2) { return c1->distCost < c2->distCost; }
    };

    typedef QVector<QVector<SCell> > TGrid;
    typedef QMultiMap<SCell *, bool> TOpenList;

    TGrid grid;
    SCell *startCell, *goalCell;
    TOpenList openList;

    QSize getGridSize(void) const;
    int getCellLength(SCell *c1, SCell *c2) const;

public:
    CPathEngine(void);

    void setGrid(const QSize &size);
    void initPath(const QPoint &start, const QPoint &goal);
    bool calcPath(QList<QPoint> &output);
    void breakConnection(const QPoint &cell, EConnection connection);
};

#endif // PATHENGINE_H

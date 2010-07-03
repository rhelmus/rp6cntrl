#include <QDebug>

#include "pathengine.h"


CPathEngine::CPathEngine() : startCell(NULL), goalCell(NULL)
{
}

QSize CPathEngine::getGridSize() const
{
    if (grid.isEmpty())
        return QSize(0, 0);

    return QSize(grid.size(), grid[0].size());
}

int CPathEngine::getCellLength(SCell *c1, SCell *c2) const
{
    // Manhatten distance
    return abs(c1->xPos - c2->xPos) + abs(c1->yPos - c2->yPos);
}

void CPathEngine::setGrid(const QSize &size)
{
    grid.clear();
    grid.resize(size.width());
    grid.fill(QVector<SCell>(size.height()));

    // Fill in coords connections
    for (int x=0; x<size.width(); ++x)
    {
        for (int y=0; y<size.height(); ++y)
        {
            grid[x][y].xPos = x;
            grid[x][y].yPos = y;

            if ((x-1) >= 0)
                grid[x][y].connections[CONNECTION_LEFT] = &grid[x-1][y];
            if ((x+1) < size.width())
                grid[x][y].connections[CONNECTION_RIGHT] = &grid[x+1][y];
            if ((y-1) >= 0)
                grid[x][y].connections[CONNECTION_UP] = &grid[x][y-1];
            if ((y+1) < size.height())
                grid[x][y].connections[CONNECTION_DOWN] = &grid[x][y+1];
        }
    }
}

void CPathEngine::initPath(const QPoint &start, const QPoint &goal)
{
    startCell = &grid[start.x()][start.y()];
    goalCell = &grid[goal.x()][goal.y()];

    const QSize gridsize(getGridSize());
    for (int x=0; x<gridsize.width(); ++x)
    {
        for (int y=0; y<gridsize.height(); ++y)
        {
            grid[x][y].score = 0;
            grid[x][y].distCost = 0;
            grid[x][y].open = grid[x][y].closed = false;
            grid[x][y].parent = NULL;
        }
    }

    openList.clear();
    openList.insert(startCell, true);
}

bool CPathEngine::calcPath(QList<QPoint> &output)
{
    const QSize gridsize = getGridSize();

    while (!openList.isEmpty())
    {
        SCell *cell = openList.begin().key();
        openList.erase(openList.begin());

        cell->open = false;
        cell->closed = true;

        if (cell == goalCell)
        {
            SCell *c = cell;
            while (c)
            {
                output.push_front(QPoint(c->xPos, c->yPos));
                c = c->parent;
            }
            return true;
        }

        for (int i=0; i<MAX_CONNECTIONS; i++)
        {
            if (!cell->connections[i])
                continue;

            SCell *child = cell->connections[i];
            int newscore = cell->score + 1; // UNDONE

            if ((child->score <= newscore) && (child->open || child->closed))
                continue;

            child->parent = cell;
            child->score = newscore;
            child->closed = false;

            // Remove any present in open list so we can change the distCost
            openList.remove(child);

            child->distCost = child->score + getCellLength(cell, child);
            child->open = true;
            openList.insert(child, true);
        }

    }

    qDebug() << "Failed to generate path!";

    return false;
}

void CPathEngine::breakConnection(const QPoint &cell, EConnection connection)
{
    qDebug() << "Breaking connection " << (int)connection;
    grid[cell.x()][cell.y()].connections[connection] = NULL;
}

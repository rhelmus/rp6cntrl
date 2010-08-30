#include <iostream>
#include <assert.h>
#include <math.h>

#include <QDebug>
#include <QTime>

#include "pathengine.h"

CPathEngine::CPathEngine() : startCell(NULL), goalCell(NULL)
{
}

void CPathEngine::initCell(int x, int y)
{
    const QSize gridsize(getGridSize());

    grid[x][y].xPos = x;
    grid[x][y].yPos = y;

    if ((x-1) >= 0)
        grid[x][y].connections[CONNECTION_LEFT].set(x-1, y);
    if ((x+1) < gridsize.width())
        grid[x][y].connections[CONNECTION_RIGHT].set(x+1, y);
    if ((y-1) >= 0)
        grid[x][y].connections[CONNECTION_UP].set(x, y-1);
    if ((y+1) < gridsize.height())
        grid[x][y].connections[CONNECTION_DOWN].set(x, y+1);
}

void CPathEngine::moveCell(SCell *cell, int dx, int dy)
{
    cell->xPos += dx;
    cell->yPos += dy;
    for (int i=0; i<MAX_CONNECTIONS; ++i)
    {
        if (!cell->connections[i].broken)
        {
            cell->connections[i].set(cell->connections[i].xPos + dx,
                                     cell->connections[i].yPos + dy);
        }
    }
}

QSize CPathEngine::getGridSize() const
{
    if (grid.isEmpty())
        return QSize(0, 0);

    return QSize(grid.size(), grid[0].size());
}

int CPathEngine::getCellLength(const SCell *c1, const SCell *c2) const
{
    // Manhatten distance
    return abs(c1->xPos - c2->xPos) + abs(c1->yPos - c2->yPos);
}

CPathEngine::EConnection CPathEngine::getCellConnection(const SCell *c1, const SCell *c2) const
{
    if (c1->connections[CONNECTION_LEFT].connected(c2->xPos, c2->yPos))
        return CONNECTION_LEFT;
    if (c1->connections[CONNECTION_RIGHT].connected(c2->xPos, c2->yPos))
        return CONNECTION_RIGHT;
    if (c1->connections[CONNECTION_UP].connected(c2->xPos, c2->yPos))
        return CONNECTION_UP;
    if (c1->connections[CONNECTION_DOWN].connected(c2->xPos, c2->yPos))
        return CONNECTION_DOWN;

    assert(false); // This really should not happen
    return CONNECTION_DOWN; // Dummy ret
}

int CPathEngine::pathScore(const SCell *c1, const SCell *c2) const
{
    // Score is based on if the bot has to turn: we prefer to travel in straight lines
    int ret = 1; // min 1

    if (c1->parent)
    {
        EConnection first = getCellConnection(c1, c2), parent = getCellConnection(c1->parent, c1);
        if (first != parent) // Changing direction?
            ret++; // Increase score
    }

    return ret;
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
            initCell(x, y);
    }
}

void CPathEngine::expandGrid(int left, int up, int right, int down)
{
    QSize gridsize(getGridSize());

    if (left)
    {
        grid.insert(0, left, QVector<SCell>(gridsize.height()));
        gridsize.rwidth() += left;
        for (int x=0; x<gridsize.width(); ++x)
        {
            for (int y=0; y<gridsize.height(); ++y)
            {
                if (x < left) // New cell?
                    initCell(x, y);
                else
                {
                    moveCell(&grid[x][y], left, 0);
                    if (x == left) // Connect with new neighbour cell
                        grid[x][y].connections[CONNECTION_LEFT].set(x-1, y);
                }
            }
        }
    }

    if (up)
    {
        gridsize.rheight() += up;

        for (int x=0; x<gridsize.width(); ++x)
        {
            grid[x].insert(0, up, SCell());

            for (int y=0; y<gridsize.height(); ++y)
            {
                if (y < up) // New cell?
                    initCell(x, y);
                else
                {
                    moveCell(&grid[x][y], 0, up);
                    if (y == up) // Connect with new neighbour cell
                        grid[x][y].connections[CONNECTION_UP].set(x, y-1);
                }
            }
        }
    }

    if (right)
    {
        grid.insert(grid.end(), right, QVector<SCell>(gridsize.height()));
        gridsize.rwidth() += right;

        // Init new cells
        for (int x=gridsize.width()-right-1; x<gridsize.width(); ++x)
        {
            for (int y=0; y<gridsize.height(); ++y)
                initCell(x, y);
        }

        // Connect with new neighbours
        const int cellx = gridsize.width() - right - 2;
        for (int y=0; y<gridsize.height(); ++y)
            grid[cellx][y].connections[CONNECTION_RIGHT].set(cellx+1, y);
    }

    if (down)
    {
        gridsize.rheight() += down;

        for (int x=0; x<gridsize.width(); ++x)
        {
            grid[x].insert(grid[x].end(), down, SCell());

            // Init new cells
            for (int y=gridsize.height()-down-1; y<gridsize.height(); ++y)
                initCell(x, y);

            // Connect with new neighbour
            const int celly = gridsize.height() - down - 2;
            grid[x][celly].connections[CONNECTION_DOWN].set(x, celly+1);
        }
    }
#if 0
    while (left)
    {
        grid.append(QVector<SCell>(gridsize.height())); // Add column
        ++gridsize.rwidth();

        // Shift cells right
        for (int x=gridsize.width()-1; x>0; --x)
        {
            for (int y=0; y<gridsize.height(); ++y)
            {
                if (x == (gridsize.width()-1))
                    initCell(x, y); // Init new cells

                QPoint &cell1 = grid[x][y], &cell2 = grid[x-1][y];
                qSwap(grid[x][y], grid[x-1][y]);
                --cell1.rx();
                ++cell2.rx();
            }
        }
        --left;
    }

    while (up)
    {
        // Shift cells to down
        for (int x=0; x<gridsize.width(); ++x)
        {
            grid[x] << SCell(); // Add row
            ++gridsize.rheight();
            initCell(x, gridsize.height()-1);

            for (int y=gridsize.height()-1; y>0; ++y)
            {
                QPoint &cell1 = grid[x][y], &cell2 = grid[x][y-1];
                qSwap(grid[x][y], grid[x-1][y]);
                --cell1.ry();
                ++cell2.ry();
            }
        }
        --up;
    }

    while (right)
    {
        grid.append(QVector<SCell>(gridsize.height())); // Add column
        ++gridsize.rwidth();

        // Init new cells
        for (int y=0; y<gridsize.height(); ++y)
            initCell(gridsize.width()-1, y);

        --right;
    }

    while (down)
    {
        for (int x=0; x<gridsize.width(); ++x)
        {
            grid[x] << SCell(); // Add row
            ++gridsize.rheight();
            initCell(x, gridsize.height()-1);
        }

        --down;
    }
#endif
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
    startCell->open = true;
    startCell->distCost = getCellLength(startCell, goalCell);

#ifdef USE_QTMAP
    openList.insert(startCell->distCost, startCell);
#else
    openList.insert(startCell);
#endif
}

bool CPathEngine::calcPath(QList<QPoint> &output)
{
    const QSize gridsize = getGridSize();

    QTime starttime;
    starttime.start();

    int investigated = 0;

#ifdef USE_QTMAP
    while (!openList.isEmpty())
#else
    while (!openList.empty())
#endif
    {
#ifdef USE_QTMAP
        SCell *cell = openList.begin().value();
#else
        SCell *cell = *openList.begin();
#endif
        openList.erase(openList.begin());

        ++investigated;

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
            qDebug() << "Path done: " << starttime.elapsed() << " ms. Investigated: " << investigated;
//            for (int y=0; y<gridsize.height(); ++y)
//            {
//                for (int x=0; x<gridsize.width(); ++x)
//                {
//                    std::cout << grid[x][y].distCost << ",";
//                }
//                std::cout << '\n';
//            }
//            std::cout << '\n' << std::flush;
            return true;
        }

        for (int i=0; i<MAX_CONNECTIONS; ++i)
        {
            if (cell->connections[i].broken)
                continue;

            SCell *child = &grid[cell->connections[i].xPos][cell->connections[i].yPos];

            if (child->closed)
                continue;

            int newscore = cell->score + pathScore(cell, child);

            if (child->open)
            {
                if (newscore >= child->score)
                    continue;

                // Remove any present in open list so we can change the distCost
#ifdef USE_QTMAP
                TOpenList::iterator it = openList.lowerBound(child->distCost);
                assert(it != openList.end());
                while ((it != openList.end()) && (it.key() == child->distCost))
                {
                    if (it.value() == child)
                    {
                        openList.erase(it);
                        break; // Assume child cannot be present more than once
                    }
                    ++it;
                }
#else
                std::pair<TOpenList::iterator, TOpenList::iterator> itpair = openList.equal_range(child);
                if ((itpair.first != openList.end()) && (itpair.second != openList.end()))
                {
                    TOpenList::iterator it = itpair.first;
                    while ((*it)->distCost == child->distCost)
                    {
                        if (*it == child)
                        {
                            // Assume only 1 duplicate can exist
                            openList.erase(it);
                            break;
                        }
                        ++it;
                    }
                }
#endif
            }

            child->parent = cell;
            child->score = newscore;
            child->closed = false;

            child->distCost = child->score + getCellLength(child, goalCell);
            child->open = true;

#ifdef USE_QTMAP
            openList.insert(child->distCost, child);
#else
            openList.insert(child);
#endif
        }

    }

    qDebug() << "Failed to generate path!";
    qDebug() << "Time: " << starttime.elapsed() << " investigated: " << investigated;

    return false;
}

void CPathEngine::breakConnection(const QPoint &cell, EConnection connection)
{
    grid[cell.x()][cell.y()].connections[connection].breakCon();
}

void CPathEngine::breakAllConnections(const QPoint &cell)
{
    for (int i=0; i<MAX_CONNECTIONS; ++i)
    {
        if (!grid[cell.x()][cell.y()].connections[i].broken)
        {
            // Break neighbour --> me
            if (i == CONNECTION_LEFT)
                grid[cell.x()-1][cell.y()].connections[CONNECTION_RIGHT].breakCon();
            else if (i == CONNECTION_RIGHT)
                grid[cell.x()+1][cell.y()].connections[CONNECTION_LEFT].breakCon();
            else if (i == CONNECTION_UP)
                grid[cell.x()][cell.y()-1].connections[CONNECTION_DOWN].breakCon();
            else if (i == CONNECTION_DOWN)
                grid[cell.x()][cell.y()+1].connections[CONNECTION_UP].breakCon();

            grid[cell.x()][cell.y()].connections[i].breakCon(); // Break me --> neighbour
        }
    }
}

#include "grid.h"

#include <cassert>

Grid::Grid(int nx, int ny, int nz, float cellSize)
    : nx(nx), ny(ny), nz(nz), cellSize(cellSize), cells(static_cast<size_t>(nx) * ny * nz)
{
    
}

GridCell &Grid::at(int i, int j, int k)
{
    return cells[static_cast<size_t>(index(i, j, k))];
}

Eigen::Vector3f Grid::cellCenter(int i, int j, int k) const
{
    return Eigen::Vector3f(
        (static_cast<float>(i) + 0.5f) * cellSize,
        (static_cast<float>(j) + 0.5f) * cellSize,
        (static_cast<float>(k) + 0.5f) * cellSize
    );
}

int Grid::index(int i, int j, int k) const
{

    return i + nx * (j + ny * k);
}

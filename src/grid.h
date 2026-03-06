#pragma once

#include <Eigen/Core>

#include <vector>

struct GridCell
{
    Eigen::Vector3f velocity = Eigen::Vector3f::Zero();
};

class Grid
{
public:
    int nx;
    int ny;
    int nz;
    float cellSize;

    std::vector<GridCell> cells;

    Grid(int nx, int ny, int nz, float cellSize);

    GridCell &at(int i, int j, int k);
    const GridCell &at(int i, int j, int k) const;

    Eigen::Vector3f cellCenter(int i, int j, int k) const;

private:
    int index(int i, int j, int k) const;
};

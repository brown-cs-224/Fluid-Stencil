#pragma once

#include "graphics/shape.h"
#include "grid.h"

class Shader;


class Simulation
{
public:
    Simulation(Grid &grid);

    void init();

    void update(double seconds);

    void draw(Shader *shader);

    void toggleWire();
private:
    Shape m_shape;
    Grid &m_grid;

    Shape m_ground;
    void initGround();
};

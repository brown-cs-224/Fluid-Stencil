#pragma once

#include "graphics/shape.h"
#include "grid.h"

class Shader;


class GridRenderer
{
    public:
        GridRenderer();

        void init();

        void update(double seconds);

        void draw(Shader *shader, const Grid &grid);
    
    private:
        Shape m_domainCube;

        Shape m_ground;
        void initDomainCube();



};

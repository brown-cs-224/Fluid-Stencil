#pragma once

#include "graphics/shape.h"

class Shader;

struct SimulationDomain {
    int nx = 32;
    int ny = 32;
    int nz = 32;
    float size = 1.0f; // world-space cube size
};

class FluidSim
{
    public:
        FluidSim();

        void init();

        void update(double seconds);

        void draw(Shader *shader);
    
    private:
        Shape m_domainCube;

        Shape m_ground;
        void initDomainCube();



};
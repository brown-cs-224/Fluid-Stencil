#pragma once

#include <GL/glew.h>

#include "graphics/shape.h"
#include "grid.h"

class Shader;

enum class GridRenderMode
{
    GRID_CENTER = 0,
    VELOC = 1,
    DENSITY = 2,
    PRESSURE = 3
};

class GridRenderer
{
    public:
        GridRenderer();

        void init();

        void update(double seconds);

        void draw(Shader *shader, const Grid &grid, GridRenderMode mode);
    
    private:
        Shape m_domainCube;
        GLuint m_pointsVao;
        GLuint m_pointsVbo;
        GLuint m_arrowsVao;
        GLuint m_arrowsVbo;
        GLuint m_sphereVao;
        GLuint m_sphereVbo;
        GLuint m_sphereIbo;
        GLuint m_instanceVbo;
        GLsizei m_numPointVertices;
        GLsizei m_numArrowVertices;
        GLsizei m_numSphereIndices;
        GLsizei m_numSphereInstances;
        float max_veloc = 0.f;
        float m_sphereScale = 0.04f;

        void initDomainCube();
        void initDebugGeometryBuffers();
        void initSphereGeometry();
        void updateDomainCubeTransform(const Grid &grid);
        void uploadCellCenterPoints(const Grid &grid);
        void uploadVelocityArrows(const Grid &grid);
        void uploadScalarSpheres(const Grid &grid, bool useDensity);
        void drawCellCenters(Shader *shader);
        void drawVelocityArrows(Shader *shader);
        void drawScalarSpheres(Shader *shader);
};

#pragma once

#include <GL/glew.h>

#include "graphics/shape.h"
#include "grid.h"

class Shader;

enum class GridRenderMode
{
    GRID_CENTER = 0,
    VELOC = 1
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
        GLsizei m_numPointVertices;
        GLsizei m_numArrowVertices;

        void initDomainCube();
        void initDebugGeometryBuffers();
        void updateDomainCubeTransform(const Grid &grid);
        void uploadCellCenterPoints(const Grid &grid);
        void uploadVelocityArrows(const Grid &grid);
        void drawCellCenters(Shader *shader);
        void drawVelocityArrows(Shader *shader);
};

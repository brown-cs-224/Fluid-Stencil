#include "gridrenderer.h"
#include "graphics/meshloader.h"

#include <iostream>

using namespace Eigen;

GridRenderer::GridRenderer() {}

void GridRenderer::init()
{
    
    initDomainCube();
}

void GridRenderer::draw(Shader *shader, const Grid &grid)
{
    m_domainCube.draw(shader);
}

void GridRenderer::update(double seconds)
{
    
}

void GridRenderer::initDomainCube()
{
    std::vector<Eigen::Vector3d> verts = {
        {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
        {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
    };

    std::vector<Eigen::Vector2i> edges = {
        {0,1},{1,2},{2,3},{3,0}, 
        {4,5},{5,6},{6,7},{7,4}, 
        {0,4},{1,5},{2,6},{3,7}  
    };

    m_domainCube.initEdges(verts, edges);

}

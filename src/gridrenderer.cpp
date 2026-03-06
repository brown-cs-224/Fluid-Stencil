#include "gridrenderer.h"
#include "graphics/shader.h"

#include <Eigen/Geometry>

#include <iostream>
#include <vector>

using namespace Eigen;

namespace
{
struct DebugVertex
{
    GLfloat px;
    GLfloat py;
    GLfloat pz;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
};

Vector3f toRenderSpaceCenter(const Grid &grid, int i, int j, int k)
{
    const float fx = (static_cast<float>(i) + 0.5f) / static_cast<float>(grid.nx);
    const float fy = (static_cast<float>(j) + 0.5f) / static_cast<float>(grid.ny);
    const float fz = (static_cast<float>(k) + 0.5f) / static_cast<float>(grid.nz);
    return Vector3f(2.f * fx - 1.f, 2.f * fy - 1.f, 2.f * fz - 1.f);
}
}

GridRenderer::GridRenderer()
    : m_pointsVao(0),
      m_pointsVbo(0),
      m_arrowsVao(0),
      m_arrowsVbo(0),
      m_numPointVertices(0),
      m_numArrowVertices(0)
{
}

void GridRenderer::init()
{
    initDomainCube();
    initDebugGeometryBuffers();
}

void GridRenderer::draw(Shader *shader, const Grid &grid, GridRenderMode mode)
{
    updateDomainCubeTransform(grid);

    if (mode == GridRenderMode::VELOC) {
        uploadVelocityArrows(grid);
    } else {
        m_numArrowVertices = 0;
        uploadCellCenterPoints(grid);
    }

    m_domainCube.draw(shader);
    if (mode == GridRenderMode::VELOC) {
        drawVelocityArrows(shader);
    }
    else{
        drawCellCenters(shader);
    }
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

void GridRenderer::initDebugGeometryBuffers()
{
    glGenVertexArrays(1, &m_pointsVao);
    glGenBuffers(1, &m_pointsVbo);

    glBindVertexArray(m_pointsVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_pointsVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &m_arrowsVao);
    glGenBuffers(1, &m_arrowsVbo);

    glBindVertexArray(m_arrowsVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_arrowsVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GridRenderer::updateDomainCubeTransform(const Grid &grid)
{
    (void)grid;
    m_domainCube.setModelMatrix(Affine3f::Identity());
}

void GridRenderer::uploadCellCenterPoints(const Grid &grid)
{
    std::vector<DebugVertex> vertices;
    vertices.reserve(static_cast<size_t>(grid.nx) * grid.ny * grid.nz);

    for (int k = 0; k < grid.nz; ++k) {
        for (int j = 0; j < grid.ny; ++j) {
            for (int i = 0; i < grid.nx; ++i) {
                const Vector3f c = toRenderSpaceCenter(grid, i, j, k);
                vertices.push_back({c.x(), c.y(), c.z(), 0.f, 1.f, 0.f});
            }
        }
    }

    m_numPointVertices = static_cast<GLsizei>(vertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_pointsVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(DebugVertex)),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GridRenderer::uploadVelocityArrows(const Grid &grid)
{
    static constexpr float kVelocityArrowScale = 0.15f;
    static constexpr float kMinVelocity = 1e-6f;

    std::vector<DebugVertex> vertices;
    vertices.reserve(static_cast<size_t>(grid.nx) * grid.ny * grid.nz * 6);

    const Vector3f domainExtents(
        static_cast<float>(grid.nx) * grid.cellSize,
        static_cast<float>(grid.ny) * grid.cellSize,
        static_cast<float>(grid.nz) * grid.cellSize
        );

    for (int k = 0; k < grid.nz; ++k) {
        for (int j = 0; j < grid.ny; ++j) {
            for (int i = 0; i < grid.nx; ++i) {

                const Vector3f start = toRenderSpaceCenter(grid, i, j, k);
                const Vector3f vel = grid.at(i, j, k).velocity;

                const Vector3f velRender(
                    domainExtents.x() > 0.f ? (2.f * vel.x() / domainExtents.x()) : 0.f,
                    domainExtents.y() > 0.f ? (2.f * vel.y() / domainExtents.y()) : 0.f,
                    domainExtents.z() > 0.f ? (2.f * vel.z() / domainExtents.z()) : 0.f
                    );

                float mag = velRender.norm();
                if (mag < kMinVelocity)
                    continue;

                Vector3f dir = velRender / mag;

                // Shaft length
                float arrowLength = mag * kVelocityArrowScale;
                Vector3f end = start + dir * arrowLength;

                // Main arrow line
                vertices.push_back({start.x(), start.y(), start.z(), vel.x(), vel.y(), vel.z()});
                vertices.push_back({end.x(), end.y(), end.z(), vel.x(), vel.y(), vel.z()});

                // ---- Arrowhead ----

                Vector3f perp(-dir.y(), dir.x(), 0.0f);
                if (perp.squaredNorm() < 1e-6f)
                    perp = Vector3f(0.0f, -dir.z(), dir.y());

                perp.normalize();

                float headLength = arrowLength * 0.25f;
                float headWidth  = arrowLength * 0.15f;

                Vector3f back = end - dir * headLength;

                Vector3f left  = back + perp * headWidth;
                Vector3f right = back - perp * headWidth;

                // Left wing
                vertices.push_back({end.x(), end.y(), end.z(), vel.x(), vel.y(), vel.z()});
                vertices.push_back({left.x(), left.y(), left.z(), vel.x(), vel.y(), vel.z()});

                // Right wing
                vertices.push_back({end.x(), end.y(), end.z(), vel.x(), vel.y(), vel.z()});
                vertices.push_back({right.x(), right.y(), right.z(), vel.x(), vel.y(), vel.z()});
            }
        }
    }

    m_numArrowVertices = static_cast<GLsizei>(vertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_arrowsVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(DebugVertex)),
        vertices.data(),
        GL_DYNAMIC_DRAW
        );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GridRenderer::drawCellCenters(Shader *shader)
{
    if (m_numPointVertices == 0) {
        return;
    }

    Eigen::Matrix4f I4 = Eigen::Matrix4f::Identity();
    Eigen::Matrix3f I3 = Eigen::Matrix3f::Identity();
    shader->setUniform("cube", 0);
    shader->setUniform("model", I4);
    shader->setUniform("inverseTransposeModel", I3);
    shader->setUniform("red", 0.2f);
    shader->setUniform("green", 0.8f);
    shader->setUniform("blue", 1.0f);
    shader->setUniform("alpha", 1.0f);

    glPointSize(2.0f);
    glBindVertexArray(m_pointsVao);
    glDrawArrays(GL_POINTS, 0, m_numPointVertices);
    glBindVertexArray(0);
}

void GridRenderer::drawVelocityArrows(Shader *shader)
{
    if (m_numArrowVertices == 0) {
        return;
    }

    Eigen::Matrix4f I4 = Eigen::Matrix4f::Identity();
    Eigen::Matrix3f I3 = Eigen::Matrix3f::Identity();
    shader->setUniform("cube", 0);
    shader->setUniform("model", I4);
    shader->setUniform("inverseTransposeModel", I3);
    shader->setUniform("red", 1.0f);
    shader->setUniform("green", 0.0f);
    shader->setUniform("blue", 0.0f);
    shader->setUniform("alpha", 1.0f);

    glBindVertexArray(m_arrowsVao);
    glDrawArrays(GL_LINES, 0, m_numArrowVertices);
    glBindVertexArray(0);
}

#include "simulation.h"
#include "graphics/meshloader.h"

#include <iostream>

using namespace Eigen;

Simulation::Simulation(Grid &grid) 
    : m_grid(grid)
{

}

void Simulation::init()
{

    const Vector3f domainCenter(
        0.5f * static_cast<float>(m_grid.nx) * m_grid.cellSize,
        0.5f * static_cast<float>(m_grid.ny) * m_grid.cellSize,
        0.5f * static_cast<float>(m_grid.nz) * m_grid.cellSize
    );

    for (int k = 0; k < m_grid.nz; ++k) {
        for (int j = 0; j < m_grid.ny; ++j) {
            for (int i = 0; i < m_grid.nx; ++i) {
                const Vector3f p = m_grid.cellCenter(i, j, k);
                const Vector3f d = p - domainCenter;
                m_grid.at(i, j, k).velocity = 0.2f * Vector3f(-d.z(), 0.f, d.x());
            }
        }
    }

    initGround();
}

void Simulation::update(double seconds)
{
    const Vector3f domainCenter(
        0.5f * m_grid.nx * m_grid.cellSize,
        0.5f * m_grid.ny * m_grid.cellSize,
        0.5f * m_grid.nz * m_grid.cellSize
        );

    static float totalTime = 0.0f;
    totalTime += static_cast<float>(seconds);

    const float rotationSpeed = 2.0f; // radians per second
    const float vortexStrength = 1.0f;

    for (int k = 0; k < m_grid.nz; ++k) {
        for (int j = 0; j < m_grid.ny; ++j) {
            for (int i = 0; i < m_grid.nx; ++i) {
                Vector3f d = m_grid.cellCenter(i, j, k) - domainCenter;

                float theta = rotationSpeed * totalTime;

                float cosTheta = cos(theta);
                float sinTheta = sin(theta);

                Vector3f tangent(-sinTheta * d.x() - cosTheta * d.z(),
                                 0.0f,
                                 cosTheta * d.x() - sinTheta * d.z());

                if (tangent.norm() > 1e-6f)
                    tangent.normalize();

                m_grid.at(i, j, k).velocity = vortexStrength * tangent;
            }
        }
    }
}

void Simulation::draw(Shader *shader)
{

}

void Simulation::toggleWire()
{
}

void Simulation::initGround()
{

}

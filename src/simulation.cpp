#include "simulation.h"

#include <algorithm>
#include <cmath>
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

    const float maxDist = 0.5f * std::sqrt(
        static_cast<float>(m_grid.nx * m_grid.nx + m_grid.ny * m_grid.ny + m_grid.nz * m_grid.nz)
    ) * m_grid.cellSize;
    const float sigma = 0.35f * maxDist;
    const float invTwoSigma2 = sigma > 0.0f ? (1.0f / (2.0f * sigma * sigma)) : 0.0f;

    for (int k = 0; k < m_grid.nz; ++k) {
        for (int j = 0; j < m_grid.ny; ++j) {
            for (int i = 0; i < m_grid.nx; ++i) {
                const Vector3f p = m_grid.cellCenter(i, j, k);
                const Vector3f d = p - domainCenter;
                m_grid.at(i, j, k).velocity = 0.2f * Vector3f(-d.z(), 0.f, d.x());

                const float radial = d.squaredNorm();
                const float density = std::exp(-radial * invTwoSigma2);
                m_grid.at(i, j, k).density = density;

                const float height = maxDist > 0.0f ? (d.y() / maxDist) : 0.0f;
                m_grid.at(i, j, k).pressure = 0.5f + 0.5f * height;
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

    const float maxDist = 0.5f * std::sqrt(
        static_cast<float>(m_grid.nx * m_grid.nx + m_grid.ny * m_grid.ny + m_grid.nz * m_grid.nz)
    ) * m_grid.cellSize;

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

                const float radial = maxDist > 0.0f ? (d.norm() / maxDist) : 0.0f;
                const float wave = std::sin(totalTime * 1.7f + radial * 6.0f);
                const float density = 0.35f + 0.65f * (0.5f + 0.5f * wave);
                m_grid.at(i, j, k).density = std::clamp(density, 0.0f, 1.0f);

                const float height = maxDist > 0.0f ? (d.y() / maxDist) : 0.0f;
                const float pressureWave = std::cos(totalTime * 1.1f + d.x() * 2.5f);
                const float pressure = 0.5f + 0.35f * height + 0.25f * pressureWave;
                m_grid.at(i, j, k).pressure = std::clamp(pressure, 0.0f, 1.0f);
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

#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

using namespace Eigen;

namespace
{
std::mt19937 &particleRng()
{
    static std::mt19937 rng(1337);
    return rng;
}

float random01()
{
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(particleRng());
}
}

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
    const float spanX = static_cast<float>(m_grid.nx) * m_grid.cellSize;
    const float spanY = static_cast<float>(m_grid.ny) * m_grid.cellSize;
    const float spanZ = static_cast<float>(m_grid.nz) * m_grid.cellSize;
    const float maxSphereRadius = 0.5f * std::min(spanX, std::min(spanY, spanZ));
    if (m_particleSpawnSphereRadius <= 0.0f) {
        m_particleSpawnSphereRadius = 0.25f * maxSphereRadius;
    }
    const float sigma = 0.35f * maxDist;
    const float invTwoSigma2 = sigma > 0.0f ? (1.0f / (2.0f * sigma * sigma)) : 0.0f;

    for (int k = 0; k < m_grid.nz; ++k) {
        for (int j = 0; j < m_grid.ny; ++j) {
            for (int i = 0; i < m_grid.nx; ++i) {
                const Vector3f p = m_grid.cellCenter(i, j, k);
                const Vector3f d = p - domainCenter;
                m_grid.at(i, j, k).velocity = 0.2f * Vector3f(0.f, 1.f, 0.f);

                //Init velocity in a xz spiral
                //m_grid.at(i, j, k).velocity = 0.2f * Vector3f(-d.z(), 0.f, d.x());

                const float radial = d.squaredNorm();
                const float density = std::exp(-radial * invTwoSigma2);
                // m_grid.at(i, j, k).density = density;

                const float height = maxDist > 0.0f ? (d.y() / maxDist) : 0.0f;
                // m_grid.at(i, j, k).pressure = 0.5f + 0.5f * height;
            }
        }
    }

    m_initialCells = m_grid.cells;
    m_totalTime = 0.0f;

    initParticles();
}

void Simulation::update(double seconds)
{
    m_totalTime += static_cast<float>(seconds);
    // vortexSim(seconds);
    advectParticles(static_cast<float>(seconds));
}

//An example simulation to
void Simulation::vortexSim(double seconds){
    const Vector3f domainCenter(
        0.5f * m_grid.nx * m_grid.cellSize,
        0.5f * m_grid.ny * m_grid.cellSize,
        0.5f * m_grid.nz * m_grid.cellSize
        );

    const float maxDist = 0.5f * std::sqrt(
        static_cast<float>(m_grid.nx * m_grid.nx + m_grid.ny * m_grid.ny + m_grid.nz * m_grid.nz)
    ) * m_grid.cellSize;

    const float rotationSpeed = 2.0f; // radians per second
    const float vortexStrength = 1.0f;

    for (int k = 0; k < m_grid.nz; ++k) {
        for (int j = 0; j < m_grid.ny; ++j) {
            for (int i = 0; i < m_grid.nx; ++i) {
                Vector3f d = m_grid.cellCenter(i, j, k) - domainCenter;

                float theta = rotationSpeed * m_totalTime;

                float cosTheta = cos(theta);
                float sinTheta = sin(theta);

                Vector3f tangent(-sinTheta * d.x() - cosTheta * d.z(),
                                 0.0f,
                                 cosTheta * d.x() - sinTheta * d.z());

                if (tangent.norm() > 1e-6f)
                    tangent.normalize();

                m_grid.at(i, j, k).velocity = vortexStrength * tangent;

                const float radial = maxDist > 0.0f ? (d.norm() / maxDist) : 0.0f;
                const float wave = std::sin(m_totalTime * 1.7f + radial * 6.0f);
                const float density = 0.35f + 0.65f * (0.5f + 0.5f * wave);
                m_grid.at(i, j, k).density = std::clamp(density, 0.0f, 1.0f);

                const float height = maxDist > 0.0f ? (d.y() / maxDist) : 0.0f;
                const float pressureWave = std::cos(m_totalTime * 1.1f + d.x() * 2.5f);
                const float pressure = 0.5f + 0.35f * height + 0.25f * pressureWave;
                m_grid.at(i, j, k).pressure = std::clamp(pressure, 0.0f, 1.0f);
            }
        }
    }

}


void Simulation::initParticles()
{
    m_particles.resize(std::max(1, m_particleCount));
    m_spawnCursor = 0;
    for (auto &particle : m_particles) {
        respawnParticle(particle);
    }
}

void Simulation::advectParticles(float seconds)
{
    if (m_particles.empty()) {
        return;
    }

    const Vector3f domainMin(0.0f, 0.0f, 0.0f);
    const Vector3f domainMax(
        static_cast<float>(m_grid.nx) * m_grid.cellSize,
        static_cast<float>(m_grid.ny) * m_grid.cellSize,
        static_cast<float>(m_grid.nz) * m_grid.cellSize
    );

    static constexpr float kMaxAge = 6.0f;
    static constexpr float kSpeedScale = 0.6f;

    for (auto &particle : m_particles) {
        const Vector3f vel = m_grid.sampleVelocity(particle.position);
        particle.position += vel * (seconds * kSpeedScale);
        particle.age += seconds;

        if (particle.age > kMaxAge
            || particle.position.x() < domainMin.x() || particle.position.y() < domainMin.y() || particle.position.z() < domainMin.z()
            || particle.position.x() > domainMax.x() || particle.position.y() > domainMax.y() || particle.position.z() > domainMax.z()) {
            respawnParticle(particle);
        }
    }
}

void Simulation::respawnParticle(Particle &particle)
{
    const Vector3f domainCenter(
        0.5f * static_cast<float>(m_grid.nx) * m_grid.cellSize,
        0.5f * static_cast<float>(m_grid.ny) * m_grid.cellSize,
        0.5f * static_cast<float>(m_grid.nz) * m_grid.cellSize
    );

    const float spanX = static_cast<float>(m_grid.nx) * m_grid.cellSize;
    const float spanY = static_cast<float>(m_grid.ny) * m_grid.cellSize;
    const float spanZ = static_cast<float>(m_grid.nz) * m_grid.cellSize;
    const float maxSphereRadius = 0.5f * std::min(spanX, std::min(spanY, spanZ));
    const float sphereRadius = std::clamp(m_particleSpawnSphereRadius, 0.0f, maxSphereRadius);

    Vector3f position = domainCenter;
    switch (m_particleSpawnMode) {
    case ParticleSpawnMode::CELL_CENTERS: {
        const int cellCount = std::max(1, m_grid.nx * m_grid.ny * m_grid.nz);
        const int idx = m_spawnCursor++ % cellCount;
        const int i = idx % m_grid.nx;
        const int j = (idx / m_grid.nx) % m_grid.ny;
        const int k = idx / (m_grid.nx * m_grid.ny);
        position = m_grid.cellCenter(i, j, k);
        break;
    }
    case ParticleSpawnMode::RANDOM_IN_CELL: {
        const int i = static_cast<int>(random01() * m_grid.nx);
        const int j = static_cast<int>(random01() * m_grid.ny);
        const int k = static_cast<int>(random01() * m_grid.nz);
        const Vector3f center = m_grid.cellCenter(i, j, k);
        const float half = 0.5f * m_grid.cellSize;
        const Vector3f jitter(
            (random01() * 2.0f - 1.0f) * half,
            (random01() * 2.0f - 1.0f) * half,
            (random01() * 2.0f - 1.0f) * half
        );
        position = center + jitter;
        break;
    }
    case ParticleSpawnMode::VORTEX_RING:
    default: {
        const float radius = 0.25f * std::min(spanX, spanZ);
        const float u = random01();
        const float v = random01();
        const float w = random01();
        const float angle = v * 6.28318530718f;
        const float r = radius * std::sqrt(u);
        const float x = domainCenter.x() + r * std::cos(angle);
        const float z = domainCenter.z() + r * std::sin(angle);
        const float y = 0.25f * spanY + w * 0.5f * spanY;
        position = Vector3f(x, y, z);
        break;
    }
    case ParticleSpawnMode::SPHERE_VOLUME: {
        const float u = random01();
        const float v = random01();
        const float w = random01();
        const float theta = 2.0f * 3.14159265359f * u;
        const float phi = std::acos(2.0f * v - 1.0f);
        const float r = sphereRadius * std::cbrt(w);
        const float sinPhi = std::sin(phi);
        const float x = r * sinPhi * std::cos(theta);
        const float y = r * std::cos(phi);
        const float z = r * sinPhi * std::sin(theta);
        position = domainCenter + Vector3f(x, y, z);
        break;
    }
    }

    particle.position = position;
    particle.age = 0.0f;
}

void Simulation::setParticleCount(int count)
{
    m_particleCount = std::max(1, count);
    resetParticles();
}

void Simulation::setParticleSpawnMode(ParticleSpawnMode mode)
{
    m_particleSpawnMode = mode;
    resetParticles();
}

void Simulation::setParticleSpawnSphereRadius(float radius)
{
    const float spanX = static_cast<float>(m_grid.nx) * m_grid.cellSize;
    const float spanY = static_cast<float>(m_grid.ny) * m_grid.cellSize;
    const float spanZ = static_cast<float>(m_grid.nz) * m_grid.cellSize;
    const float maxSphereRadius = 0.5f * std::min(spanX, std::min(spanY, spanZ));
    m_particleSpawnSphereRadius = std::clamp(radius, 0.0f, maxSphereRadius);
    if (m_particleSpawnMode == ParticleSpawnMode::SPHERE_VOLUME) {
        resetParticles();
    }
}

void Simulation::resetParticles()
{
    m_particles.clear();
    initParticles();
}

void Simulation::resetGridToInitial()
{
    if (m_initialCells.empty()) {
        return;
    }

    if (m_initialCells.size() != m_grid.cells.size()) {
        m_grid.cells = m_initialCells;
    } else {
        std::copy(m_initialCells.begin(), m_initialCells.end(), m_grid.cells.begin());
    }

    m_totalTime = 0.0f;
}

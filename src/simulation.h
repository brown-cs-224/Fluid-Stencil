#pragma once

#include "graphics/shape.h"
#include "grid.h"

#include <vector>

class Shader;

struct Particle
{
    Eigen::Vector3f position = Eigen::Vector3f::Zero();
    float age = 0.0f;
};

enum class ParticleSpawnMode
{
    VORTEX_RING = 0,
    CELL_CENTERS = 1,
    RANDOM_IN_CELL = 2,
    SPHERE_VOLUME = 3
};

class Simulation
{
public:
    Simulation(Grid &grid);

    void init();

    void update(double seconds);

    const std::vector<Particle> &particles() const { return m_particles; }
    int particleCount() const { return m_particleCount; }
    ParticleSpawnMode particleSpawnMode() const { return m_particleSpawnMode; }
    float particleSpawnSphereRadius() const { return m_particleSpawnSphereRadius; }
    void setParticleCount(int count);
    void setParticleSpawnMode(ParticleSpawnMode mode);
    void setParticleSpawnSphereRadius(float radius);
    void resetParticles();
    void resetGridToInitial();
private:
    Shape m_shape;
    Grid &m_grid;

    Shape m_ground;
    void vortexSim(double seconds);

    void initParticles();
    void advectParticles(float seconds);
    void respawnParticle(Particle &particle);

    std::vector<Particle> m_particles;
    int m_particleCount = 2000;
    ParticleSpawnMode m_particleSpawnMode = ParticleSpawnMode::VORTEX_RING;
    int m_spawnCursor = 0;
    float m_particleSpawnSphereRadius = 0.0f;
    std::vector<GridCell> m_initialCells;
    float m_totalTime = 0.0f;
};

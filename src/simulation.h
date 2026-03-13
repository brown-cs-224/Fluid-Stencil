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
    RANDOM_IN_CELL = 2
};

class Simulation
{
public:
    Simulation(Grid &grid);

    void init();

    void update(double seconds);

    void draw(Shader *shader);

    void toggleWire();

    const std::vector<Particle> &particles() const { return m_particles; }
    int particleCount() const { return m_particleCount; }
    ParticleSpawnMode particleSpawnMode() const { return m_particleSpawnMode; }
    void setParticleCount(int count);
    void setParticleSpawnMode(ParticleSpawnMode mode);
    void resetParticles();
private:
    Shape m_shape;
    Grid &m_grid;

    Shape m_ground;
    void initGround();

    void initParticles();
    void advectParticles(float seconds);
    void respawnParticle(Particle &particle);

    std::vector<Particle> m_particles;
    int m_particleCount = 2000;
    ParticleSpawnMode m_particleSpawnMode = ParticleSpawnMode::VORTEX_RING;
    int m_spawnCursor = 0;
};

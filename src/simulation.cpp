#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

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

// ============================================================================
//  Stable Fluids helpers  (Stam, SIGGRAPH 1999)
// ============================================================================

// ---------------------------------------------------------------------------
// Trilinear interpolation of a flat velocity buffer.
//
// Convention: cell (i,j,k) has its centre at
//     pos = ((i+0.5)*h,  (j+0.5)*h,  (k+0.5)*h)
// Inverse mapping (world → continuous cell index):
//     gx = pos.x/h  –  0.5
// ---------------------------------------------------------------------------
Vector3f sampleBuffer(const std::vector<Vector3f>& buf,
                      int nx, int ny, int nz, float h,
                      const Vector3f& pos)
{
    float gx = pos.x() / h - 0.5f;
    float gy = pos.y() / h - 0.5f;
    float gz = pos.z() / h - 0.5f;

    // Clamp so we never read outside the array.
    gx = std::clamp(gx, 0.0f, static_cast<float>(nx - 1));
    gy = std::clamp(gy, 0.0f, static_cast<float>(ny - 1));
    gz = std::clamp(gz, 0.0f, static_cast<float>(nz - 1));

    const int i0 = static_cast<int>(gx);
    const int j0 = static_cast<int>(gy);
    const int k0 = static_cast<int>(gz);
    const int i1 = std::min(i0 + 1, nx - 1);
    const int j1 = std::min(j0 + 1, ny - 1);
    const int k1 = std::min(k0 + 1, nz - 1);

    const float fx = gx - i0;
    const float fy = gy - j0;
    const float fz = gz - k0;

    // Alias for readability – no extra allocation.
    auto at = [&](int ii, int jj, int kk) -> const Vector3f& {
        return buf[ii + jj * nx + kk * nx * ny];
    };

    // Standard trilinear blend
    return (1.0f - fz) * (
               (1.0f - fy) * ((1.0f - fx) * at(i0, j0, k0) + fx * at(i1, j0, k0)) +
                       fy  * ((1.0f - fx) * at(i0, j1, k0) + fx * at(i1, j1, k0))
           ) +
           fz * (
               (1.0f - fy) * ((1.0f - fx) * at(i0, j0, k1) + fx * at(i1, j0, k1)) +
                       fy  * ((1.0f - fx) * at(i0, j1, k1) + fx * at(i1, j1, k1))
           );
}

// ---------------------------------------------------------------------------
// No-slip boundary condition: zero the *normal* velocity component at every
// grid face.  Applied after each sub-step to keep fluid from passing through
// walls.
// ---------------------------------------------------------------------------
void enforceBoundary(std::vector<Vector3f>& vel, int nx, int ny, int nz)
{
    auto idx = [&](int i, int j, int k) { return i + j * nx + k * nx * ny; };

    for (int k = 0; k < nz; ++k)
        for (int j = 0; j < ny; ++j)
            for (int i = 0; i < nx; ++i) {
                Vector3f& v = vel[idx(i, j, k)];
                if (i == 0 || i == nx - 1) v.x() = 0.0f;  // x-walls: zero normal (x) component
                if (j == 0 || j == ny - 1) v.y() = 0.0f;  // y-walls: zero normal (y) component
                if (k == 0 || k == nz - 1) v.z() = 0.0f;  // z-walls: zero normal (z) component
            }
}

// ---------------------------------------------------------------------------
// vstep – one full Stam velocity-field update.
//
// Pipeline (Stam §2.2, Fig. 1):
//   w0  --[add force]-->  w1  --[advect]-->  w2  --[diffuse]-->  w3  --[project]-->  w4
//
// Parameters
//   grid      : current simulation grid (read + written in place)
//   dt        : time step (seconds)
//   viscosity : kinematic viscosity ν  (0 = inviscid; increase for thick fluids)
// ---------------------------------------------------------------------------
void vstep(Grid& grid, float dt, float viscosity)
{
    const int   nx = grid.nx;
    const int   ny = grid.ny;
    const int   nz = grid.nz;
    const float h  = grid.cellSize;
    const int   N  = nx * ny * nz;

    auto idx = [&](int i, int j, int k) { return i + j * nx + k * nx * ny; };

    // ----------------------------------------------------------------
    // w0 : copy the current grid velocity into a working flat buffer
    // ----------------------------------------------------------------
    std::vector<Vector3f> w(N);
    for (int k = 0; k < nz; ++k)
        for (int j = 0; j < ny; ++j)
            for (int i = 0; i < nx; ++i)
                w[idx(i, j, k)] = grid.at(i, j, k).velocity;

    // ================================================================
    // STEP 1 – Add external forces
    // ================================================================
    // w1(x) = w0(x) + dt * f(x, t)          (Stam §2.2, "add force" step)
    //
    // Gravity points in –y. This is a body force that acts on every cell
    // uniformly, regardless of density.  Because it is a spatially constant
    // vector it is already divergence-free, so it won't fight the projection
    // step – it simply shifts the magnitude of the velocity field downward.
    //
    // Tune kGravity to taste; 9.8 m/s² with typical cell sizes gives a
    // visually convincing fall that overrides the initial upward field in
    // roughly 0.2 s of real time.
    // ================================================================
    static constexpr float kGravity = 9.8f;
    const Vector3f gravity(0.0f, -kGravity, 0.0f);

    for (auto& v : w)
        v += dt * gravity;

    enforceBoundary(w, nx, ny, nz);

    // ================================================================
    // STEP 2 – Self-advection via semi-Lagrangian back-trace
    // ================================================================
    // The advection term  –(u·∇)u  moves the velocity field with itself.
    //
    // For each grid-cell centre x we ask: "where was the fluid parcel
    // that *arrives* at x at time t+dt when it was at time t?"
    //
    //   x₀ = x  –  dt · u(x)        ← backward in time (MINUS sign!)
    //
    // The new velocity at x is whatever the fluid had at its old location:
    //   w2(x) = w1(x₀)
    //
    // Because we are only *reading* old values and never extrapolating, the
    // maximum of w2 is bounded by the maximum of w1 → unconditionally stable.
    //
    // We use a second-order Runge-Kutta (midpoint) trace for accuracy:
    //   v1  = u(x)               sample at current cell
    //   mid = x – 0.5·dt·v1      half-step back
    //   v2  = u(mid)             sample at mid-point
    //   x₀  = x – dt·v2          full-step back using mid-point slope
    // ================================================================
    {
        std::vector<Vector3f> w2(N);
        for (int k = 0; k < nz; ++k) {
            for (int j = 0; j < ny; ++j) {
                for (int i = 0; i < nx; ++i) {
                    const Vector3f x   = grid.cellCenter(i, j, k);

                    // RK2 back-trace  (both steps go BACKWARD → minus signs)
                    const Vector3f v1  = sampleBuffer(w, nx, ny, nz, h, x);
                    const Vector3f mid = x - 0.5f * dt * v1;
                    const Vector3f v2  = sampleBuffer(w, nx, ny, nz, h, mid);
                    const Vector3f x0  = x - dt * v2;

                    w2[idx(i, j, k)] = sampleBuffer(w, nx, ny, nz, h, x0);
                }
            }
        }
        w = w2;
    }
    enforceBoundary(w, nx, ny, nz);

    // ================================================================
    // STEP 3 – Viscous diffusion  (implicit Gauss-Seidel)
    // ================================================================
    // The diffusion term  ν∇²u  spreads velocity, causing viscous damping.
    //
    // An *explicit* step  w3 = w2 + dt·ν·∇²w2  is only stable when
    //   dt < h² / (2ν·NDIM)
    // which restricts the time step for large ν.
    //
    // Instead we use the *implicit* form (Stam §2.2):
    //   (I – ν·dt·∇²) w3 = w2
    //
    // Discretising ∇² with central differences and isolating w3[i,j,k]:
    //
    //   w3[i,j,k] · (1 + 6a) – a · Σ_{nb} w3[nb]  =  w2[i,j,k]
    //
    //   ⟹  w3[i,j,k] = ( w2[i,j,k] + a · Σ_{nb} w3[nb] ) / (1 + 6a)
    //
    // where  a = ν·dt / h²  and the sum runs over the 6 face-neighbours.
    //
    // We iterate this formula (Gauss-Seidel) until approximate convergence.
    // It is unconditionally stable regardless of ν or dt.
    // ================================================================
    if (viscosity > 0.0f) {
        const float a   = viscosity * dt / (h * h);
        const float inv = 1.0f / (1.0f + 6.0f * a);

        std::vector<Vector3f> w3 = w;   // initial guess = w2

        for (int iter = 0; iter < 20; ++iter) {
            // Only update interior cells; boundary cells stay at zero
            // (enforced below) – this implements the Dirichlet zero-velocity BC.
            for (int k = 1; k < nz - 1; ++k) {
                for (int j = 1; j < ny - 1; ++j) {
                    for (int i = 1; i < nx - 1; ++i) {
                        const Vector3f nb =
                            w3[idx(i + 1, j,     k    )] + w3[idx(i - 1, j,     k    )] +
                            w3[idx(i,     j + 1, k    )] + w3[idx(i,     j - 1, k    )] +
                            w3[idx(i,     j,     k + 1)] + w3[idx(i,     j,     k - 1)];
                        w3[idx(i, j, k)] = (w[idx(i, j, k)] + a * nb) * inv;
                    }
                }
            }
        }
        w = w3;
        enforceBoundary(w, nx, ny, nz);
    }

    // ================================================================
    // STEP 4 – Pressure projection  (enforce ∇·u = 0)
    // ================================================================
    // The Helmholtz-Hodge Decomposition (Stam §2.1) states that any
    // vector field w can be uniquely split as:
    //
    //   w = u + ∇q        where  ∇·u = 0
    //
    // So to obtain the divergence-free part u we:
    //
    //   (a) Measure how far w3 departs from divergence-free:
    //            div[i,j,k] = ∇·w3  (central differences)
    //
    //   (b) Solve the Poisson equation for the scalar potential q:
    //            ∇²q = div
    //       Discretised:  (Σ q_nb – 6·q[i,j,k]) / h² = div[i,j,k]
    //       Isolating:     q[i,j,k] = ( Σ q_nb – h²·div[i,j,k] ) / 6
    //       → Gauss-Seidel iterations until converged.
    //
    //   (c) Subtract the gradient of q to recover the divergence-free field:
    //            w4 = w3 – ∇q
    //       Central-difference gradient:
    //            (∇q)_x = (q[i+1] – q[i-1]) / (2h)   (and similarly y, z)
    //
    // Boundary condition on q: Neumann  ∂q/∂n = 0 at walls, which is
    // automatically satisfied when GS uses the boundary cell itself as its
    // own "outside" neighbour (i.e. the clamped-index trick).  Combined with
    // enforceBoundary afterwards, this removes any flux through the walls.
    // ================================================================
    {
        std::vector<float> div(N, 0.0f);
        std::vector<float> q  (N, 0.0f);   // initial guess q = 0

        const float inv2h = 0.5f / h;

        // ---- (a) Divergence of w3 ----
        // Only interior cells: boundary velocities are already zeroed, so
        // they naturally contribute zero divergence at the faces.
        for (int k = 1; k < nz - 1; ++k) {
            for (int j = 1; j < ny - 1; ++j) {
                for (int i = 1; i < nx - 1; ++i) {
                    //  ∂u/∂x  +  ∂v/∂y  +  ∂w/∂z   (central differences, spacing h)
                    div[idx(i, j, k)] =
                        (w[idx(i + 1, j,     k    )].x() - w[idx(i - 1, j,     k    )].x()) * inv2h +
                        (w[idx(i,     j + 1, k    )].y() - w[idx(i,     j - 1, k    )].y()) * inv2h +
                        (w[idx(i,     j,     k + 1)].z() - w[idx(i,     j,     k - 1)].z()) * inv2h;
                }
            }
        }

        // ---- (b) Poisson solve  ∇²q = div  via Gauss-Seidel ----
        // Derivation of the update formula:
        //   (q[i+1] + q[i-1] + q[j+1] + q[j-1] + q[k+1] + q[k-1] – 6·q[i,j,k]) / h²
        //       = div[i,j,k]
        //
        //   q[i,j,k] = ( Σ q_nb  –  h² · div[i,j,k] ) / 6
        //
        // Note the sign: div is positive when fluid is diverging (spreading out),
        // so we subtract h²·div to push q higher where fluid is expanding, and
        // the gradient we remove in step (c) will pull velocity inward → mass
        // conservation restored.
        const float h2 = h * h;
        for (int iter = 0; iter < 40; ++iter) {
            for (int k = 1; k < nz - 1; ++k) {
                for (int j = 1; j < ny - 1; ++j) {
                    for (int i = 1; i < nx - 1; ++i) {
                        const float qnb =
                            q[idx(i + 1, j,     k    )] + q[idx(i - 1, j,     k    )] +
                            q[idx(i,     j + 1, k    )] + q[idx(i,     j - 1, k    )] +
                            q[idx(i,     j,     k + 1)] + q[idx(i,     j,     k - 1)];
                        q[idx(i, j, k)] = (qnb - h2 * div[idx(i, j, k)]) / 6.0f;
                    }
                }
            }
        }

        // ---- (c) Subtract ∇q  from w3  to obtain divergence-free  w4 ----
        // w4[i,j,k] = w3[i,j,k]  –  ∇q[i,j,k]
        //
        // Central-difference gradient (sign matters: plus for each component
        // in the positive direction, minus in the negative direction):
        //   (∇q)_x[i,j,k] = (q[i+1,j,k] – q[i-1,j,k]) / (2h)
        for (int k = 1; k < nz - 1; ++k) {
            for (int j = 1; j < ny - 1; ++j) {
                for (int i = 1; i < nx - 1; ++i) {
                    w[idx(i, j, k)] -= Vector3f(
                        (q[idx(i + 1, j,     k    )] - q[idx(i - 1, j,     k    )]) * inv2h,
                        (q[idx(i,     j + 1, k    )] - q[idx(i,     j - 1, k    )]) * inv2h,
                        (q[idx(i,     j,     k + 1)] - q[idx(i,     j,     k - 1)]) * inv2h
                    );
                }
            }
        }
        enforceBoundary(w, nx, ny, nz);
    }

    // ----------------------------------------------------------------
    // Write the final divergence-free velocity field w4 back to grid
    // ----------------------------------------------------------------
    for (int k = 0; k < nz; ++k)
        for (int j = 0; j < ny; ++j)
            for (int i = 0; i < nx; ++i)
                grid.at(i, j, k).velocity = w[idx(i, j, k)];
}

} // anonymous namespace

// ============================================================================
//  Simulation class methods
// ============================================================================

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

                // Initial velocity: uniform upward field.
                // Gravity will progressively flip this downward once vstep runs.
                // Swirling field around Y-axis (vortex)
                Vector3f swirl(-d.z(), 0.0f, d.x());

                // Avoid blow-up near center
                float r = std::sqrt(d.x() * d.x() + d.z() * d.z());
                if (r > 1e-5f) {
                    swirl /= r;  // normalize tangential direction
                }

                // Strength falls off with distance
                float strength = std::exp(-r * 2.0f);

                

                m_grid.at(i, j, k).velocity = 0.5f * strength * swirl;
                m_grid.at(i, j, k).velocity = 0.2f * Vector3f(0.f, 1.f, 0.f);

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

// ---------------------------------------------------------------------------
// update – called every frame.
//
// We run the full Stam pipeline first (velocity field update), then advect
// the visualisation particles through the resulting divergence-free field.
//
// Viscosity guide:
//   0.0f           – inviscid; swirls persist the longest (smoke / gas)
//   1e-5 … 1e-3   – low viscosity (water-like)
//   0.01 … 0.1    – medium viscosity (oil-like)
//   > 0.5          – very thick / glue-like
// ---------------------------------------------------------------------------
void Simulation::update(double seconds)
{
    m_totalTime += static_cast<float>(seconds);
    const float dt = static_cast<float>(seconds);

    // Kinematic viscosity ν.  0 = inviscid (diffusion step is skipped entirely,
    // saving work and preserving swirling detail).
    static constexpr float kViscosity = 0.0f;

    // Run the four-step Stam velocity update.
    vstep(m_grid, dt, kViscosity);

    // Advect visual particles through the now-updated divergence-free field.
    advectParticles(dt);
}

// ---------------------------------------------------------------------------
// vortexSim – kept as a reference / diagnostic tool.
// Not physically plausible; useful for understanding the rendering pipeline.
// ---------------------------------------------------------------------------
void Simulation::vortexSim(double seconds)
{
    const Vector3f domainCenter(
        0.5f * m_grid.nx * m_grid.cellSize,
        0.5f * m_grid.ny * m_grid.cellSize,
        0.5f * m_grid.nz * m_grid.cellSize
    );

    const float maxDist = 0.5f * std::sqrt(
        static_cast<float>(m_grid.nx * m_grid.nx + m_grid.ny * m_grid.ny + m_grid.nz * m_grid.nz)
    ) * m_grid.cellSize;

    const float rotationSpeed = 2.0f;
    const float vortexStrength = 1.0f;

    for (int k = 0; k < m_grid.nz; ++k) {
        for (int j = 0; j < m_grid.ny; ++j) {
            for (int i = 0; i < m_grid.nx; ++i) {
                Vector3f d = m_grid.cellCenter(i, j, k) - domainCenter;

                float theta    = rotationSpeed * m_totalTime;
                float cosTheta = cos(theta);
                float sinTheta = sin(theta);

                Vector3f tangent(-sinTheta * d.x() - cosTheta * d.z(),
                                  0.0f,
                                  cosTheta * d.x() - sinTheta * d.z());
                if (tangent.norm() > 1e-6f)
                    tangent.normalize();

                m_grid.at(i, j, k).velocity = vortexStrength * tangent;

                const float radial  = maxDist > 0.0f ? (d.norm() / maxDist) : 0.0f;
                const float wave    = std::sin(m_totalTime * 1.7f + radial * 6.0f);
                m_grid.at(i, j, k).density =
                    std::clamp(0.35f + 0.65f * (0.5f + 0.5f * wave), 0.0f, 1.0f);

                const float pressureWave = std::cos(m_totalTime * 1.1f + d.x() * 2.5f);
                m_grid.at(i, j, k).pressure =
                    std::clamp(0.5f + 0.35f * (d.y() / maxDist) + 0.25f * pressureWave, 0.0f, 1.0f);
            }
        }
    }
}

// ============================================================================
//  Particle management  (unchanged)
// ============================================================================

void Simulation::initParticles()
{
    m_particles.resize(std::max(1, m_particleCount));
    m_spawnCursor = 0;
    for (auto &particle : m_particles)
        respawnParticle(particle);
}

void Simulation::advectParticles(float seconds)
{
    if (m_particles.empty())
        return;

    const Vector3f domainMin(0.0f, 0.0f, 0.0f);
    const Vector3f domainMax(
        static_cast<float>(m_grid.nx) * m_grid.cellSize,
        static_cast<float>(m_grid.ny) * m_grid.cellSize,
        static_cast<float>(m_grid.nz) * m_grid.cellSize
    );

    static constexpr float kMaxAge    = 6.0f;
    static constexpr float kSpeedScale = 0.6f;

    for (auto &particle : m_particles) {
        // sampleVelocity reads the grid that vstep just updated.
        const Vector3f vel = m_grid.sampleVelocity(particle.position);
        particle.position += vel * (seconds * kSpeedScale);
        particle.age      += seconds;

        if (particle.age > kMaxAge
            || particle.position.x() < domainMin.x()
            || particle.position.y() < domainMin.y()
            || particle.position.z() < domainMin.z()
            || particle.position.x() > domainMax.x()
            || particle.position.y() > domainMax.y()
            || particle.position.z() > domainMax.z())
        {
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
    const float sphereRadius    = std::clamp(m_particleSpawnSphereRadius, 0.0f, maxSphereRadius);

    Vector3f position = domainCenter;
    switch (m_particleSpawnMode) {
    case ParticleSpawnMode::CELL_CENTERS: {
        const int cellCount = std::max(1, m_grid.nx * m_grid.ny * m_grid.nz);
        const int id = m_spawnCursor++ % cellCount;
        const int i = id % m_grid.nx;
        const int j = (id / m_grid.nx) % m_grid.ny;
        const int k = id / (m_grid.nx * m_grid.ny);
        position = m_grid.cellCenter(i, j, k);
        break;
    }
    case ParticleSpawnMode::RANDOM_IN_CELL: {
        const int i = static_cast<int>(random01() * m_grid.nx);
        const int j = static_cast<int>(random01() * m_grid.ny);
        const int k = static_cast<int>(random01() * m_grid.nz);
        const Vector3f centre = m_grid.cellCenter(i, j, k);
        const float half = 0.5f * m_grid.cellSize;
        position = centre + Vector3f(
            (random01() * 2.0f - 1.0f) * half,
            (random01() * 2.0f - 1.0f) * half,
            (random01() * 2.0f - 1.0f) * half
        );
        break;
    }
    case ParticleSpawnMode::VORTEX_RING:
    default: {
        const float radius = 0.25f * std::min(spanX, spanZ);
        const float u      = random01();
        const float v      = random01();
        const float w      = random01();
        const float angle  = v * 6.28318530718f;
        const float r      = radius * std::sqrt(u);
        position = Vector3f(
            domainCenter.x() + r * std::cos(angle),
            0.25f * spanY + w * 0.5f * spanY,
            domainCenter.z() + r * std::sin(angle)
        );
        break;
    }
    case ParticleSpawnMode::SPHERE_VOLUME: {
        const float u     = random01();
        const float v     = random01();
        const float w     = random01();
        const float theta = 2.0f * 3.14159265359f * u;
        const float phi   = std::acos(2.0f * v - 1.0f);
        const float r     = sphereRadius * std::cbrt(w);
        const float sp    = std::sin(phi);
        position = domainCenter + Vector3f(
            r * sp         * std::cos(theta),
            r * std::cos(phi),
            r * sp         * std::sin(theta)
        );
        break;
    }
    }

    particle.position = position;
    particle.age      = 0.0f;
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
    if (m_particleSpawnMode == ParticleSpawnMode::SPHERE_VOLUME)
        resetParticles();
}

void Simulation::resetParticles()
{
    m_particles.clear();
    initParticles();
}

void Simulation::resetGridToInitial()
{
    if (m_initialCells.empty())
        return;

    if (m_initialCells.size() != m_grid.cells.size())
        m_grid.cells = m_initialCells;
    else
        std::copy(m_initialCells.begin(), m_initialCells.end(), m_grid.cells.begin());

    m_totalTime = 0.0f;
}
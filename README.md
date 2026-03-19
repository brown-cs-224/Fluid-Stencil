# Assignment 4: Simulation (Sim) -- Fluids Option

In this assignment, you'll animate a gaseous incompressible fluid by simulating the evolution of a fluid velocity field on an Eulerian grid. Following the "Stable Fluids" paper, you'll implement external forces, advection, diffusion, and a pressure projection step which maintains incompressibility. To show off what your code can do, you’ll submit one or more videos demonstrating your simulator in action.

## Relevant Reading

- The lecture slides!
- [Baraff and Witkin’s course notes](https://www.cs.cmu.edu/~baraff/sigcourse/) on physically-based modeling are a good reference for the basics of dynamics and time integration.
- [Stable fluids](https://pages.cs.wisc.edu/~chaol/data/cs777/stam-stable_fluids.pdf) by Jos Stam

## Requirements

This assignment is out of **100 points**.

Your simulator must implement at least the following features:

- External forces:
    - **(5 points)** A gravitational force that causes the fluid to settle near the bottom of the volume over time.
    - **(5 points)**: At least one other form of external force. This could be an impulse force in response to a user click/keypress, or anything else that results in visually-interesting motion.
- Semi-Lagrangian advection:
    - **(5 points)** Backtracing according to the current velocity field.
    - **(5 points)** Trilinear interpolation to compute the value of the velocity field at the backtraced location.
- Diffusion:
    - **(15 points)** Set up the sparse linear system
    - **(10 points)** Solve the sparse linear system
- Pressure projection
    - **(10 points)** Set up the sparse linear system
    - **(10 points)** Solve the sparse linear system and subtract the gradient of the solution from the velocity field
- Boundary conditions:
    - **(10 points)** Ensure the normal component of the velocity field is zero at the boundaries of the simulation volume, so fluid does not 'escape' the volume.
- Particle advection:
    - **(5 points)** Advect particles through the velocity field and visualize them (the stencil code is set up for the latter; see below).
- Video **(10 points)**
  - You must submit at least one video demonstrating your simulator in action. The video(s) must demonstrate all of the features you have implemented (including any extra features). Particularly creative and/or nicely-rendered animations may receive extra credit. Think about interesting scenarios you could set up. Please use a standard format and codec for your video files (e.g. .mp4 with the H264 codec).
    - To make this video, we recommend screen capturing an OpenGL rendering of your simulation, e.g. using the interactive viewer code that we provide (see “Starter Code” below).
    - You may also chose to render your simulation using external software (e.g. Blender, Houdini), for which you can receive some extra credit (see "Extra Features" below).
  - To turn a set of frame images into a video, you can use [FFMPEG](https://hamelot.io/visualization/using-ffmpeg-to-convert-a-set-of-images-into-a-video/).
- README **(5 points)**
  - Your README should explain your logic for
    - the additional external force(s) you implemented
    - your implementation of particle backtracing
    - your implementation of boundary conditions
    - and any extra features you chose to implement
  - Explanations should be 3 sentences each maximum
  - Link the starting lines (on GitHub) of your implementation of these features
    - The 'add external forces step'
    - The semi-Lagrangian advection step
    - The diffusion step
    - The pressure projection step
    - Where boundary conditions are defined
  - You should also embed your videos into the README file

Successfully implementing all of the requirements results in a total of **90/100 points**.
To score **100/100** (or more!), you’ll need to implement some extra features.

### Extra Features

Each of the following features that you implement will earn you extra points. The features are ordered roughly by difficulty of implementation.

- **(5 points)** Make a cool alternative initial set of particles (in the shape of the Stanford bunny, perhaps?) and share it with the class on Slack (as e.g. a .ply or .xyz) file.
- **(up to 10 points)** Better rendering. For example, export data from your simulator (particles, densities) and render it with Blender/Houdini/etc. 
- **(5 points)** Implement additional types of external forces. Each must be nontrivially different from all other types of external forces you have implemented.
- **(up to 10 points)** Implement different boundary conditions. For example, the periodic boundary conditions mentioned in the Stable fluids paper.
- **(10 points)** Add obstacles inside the volume that the fluid must flow around. The simplest way to do this is to make certain interior grid cells be treated as boundary cells.
- **(10 points)** Use the Marker-and-Cell (MAC) grid representation, also called the staggered grid. In this representation, pressures are still stored at cell centers, but velocities are now stored on the *faces* of cells. This representation can eliminate some visual artefacts that can occur with the standard grid representation, and it also suffers a bit less from numerical dissipation.
- Something else!
  - This list is not meant to be exhaustive--if you’ve got another advanced feature in mind, go for it! (though you may want to ask a TA or the instructor first if you’re concerned about whether the idea is feasible)

**Any extra features you implement must be mentioned in your README and demonstrated in your video (you can submit multiple videos to show off different features, if that’s easier).**

## Starter Code

The starter code in this repo provides a Grid data structure and a 3D viewer for visualizing your fluid simulations. **After cloning the repo locally, you'll need to run `git submodule update --init --recursive` to update the submodules.**

### m_Grid

Look at `src/grid.h`. This class represents the grid that you will update. It is initialized, in `src/window.cpp`, with a dimension of 16 x 16 x 16 (feel free to change this to higher resolutions). The grid object is passed by reference to your simulation class (`src/simulation.cpp`). You should write the final velocity into this grid. The same reference is then passed to the renderer (`src/gridrenderer.cpp`), which visualizes the current state of your field.

#### IMPORTANT 
The velocity field stored in the grid cells of m_grid is what gets rendered and is used to advect the visualization particles. However, your simulation does not necessarily need to compute velocity in this exact representation internally. 

Fluid quantities can be stored on grids in different ways. In a collocated grid, values like velocity and pressure are stored at the center of each grid cell. The grid provided in this assignment uses a collocated grid, so the velocity stored in each GridCell represents the cell-centered velocity.

Another common approach is a MAC (Marker-and-Cell) grid, where velocity components are stored on the faces of cells instead of the center. This layout is often used in fluid solvers to better enforce incompressibility.

For this assignment, the velocity stored in m_grid is what will be rendered and used to advect particles.

The grid is initialized with default values for velocity. You will also note that there are other fields in the struct. Look at the debugging section to see how they might be useful! You don't have to explicitly update these values during your sim but they can be useful for debugging!

#### How do I update the grid?

You'll want to look at `src/simulation.cpp` to get started, as that's the only file you need to change (although you'll probably make several of your own new files, too).
You also might want to look at `src/window.cpp`, if you're interested in adding new interactivity/controls to the program (specifically for adding external forces). `src/gridrenderer.cpp` handles rendering!

The update function (in `src/simulation.cpp`) gets called every frame, and your grid gets updated here.

Look at the vortexSim() function in `src/simulation.cpp`. This function does not update the grid in a physically plausible way, but it can be a good way of understanding how to update the grid. Comment in the call to this function in the update function (in `src/simulation.cpp`). You will be able to render the updated field. Spawn some particles to get a feel for advection. Again, this is not a physically plausible simulation; it is your goal to update our field in a physically plausible way by solving the Navier Stokes Equation.

### The Visualizer

#### The Render Modes:
- Grid Centers: Visualizes the center of the grid cells
- Velocity (Important): Visualizes the velocity field of the grid for the simulation
- Debug Density: Visualizes the density field of the grid for debugging 
- Debug Pressure: Visualizes the pressure field of the grid for debugging

#### Particle Settings:
- Turn particles on/off, overlay velocity, set particle count
- Play around with particle initialization. Sphere mode spawns particles in a sphere of adjustable radius

#### Key Controls:

Note: You might need to click on the render window after interacting with the UI for keys to work.
- Move Camera: WASD
- Look around: Click and hold mouse and drag
- Toggle orbit mode: (changes the camera from a first-person view to an orbiting camera a la what the Maya editor does)
- SPACE: Respawns particles for advection
- P: Pauses the simulation
- O: Resets the simulation to the initial velocity condition (so that you can start your simulation over!)

When the program first loads, you should see a velocity field pointing up along the Y-Axis. You can spawn particles to see them get advected along this velocity field. Play around with the particle initializations!

### Implementation Tips:

- We recommend implementing the terms in the following order:
    - Forces
    - Advection
    - Projection
    - Diffusion
- Start with the existing Collocated Grid for your simulation (velocity at grid center). If you get this working, you may consider moving to the MAC representation.
- Start simple. Begin by applying a constant force, such as gravity, to the velocity field.
- Test gravity first. If gravity is implemented correctly, particles should gradually accelerate downward and settle toward the bottom of the simulation domain. Remember to apply the boundary conditions!
- Build the solver incrementally. Implement and test each step of the algorithm (forces, advection, viscosity, projection) separately before combining them.

### Debugging Tips:

- You will notice that alongside velocity, the grid can store two extra fields. While the simulation only computes velocity, **explicitly**, we can use these other properties to debug our fluid simulation.

- Pressure: During the projection step of the Stable Fluids algorithm, you solve a Poisson equation to compute a pressure field that removes divergence from the velocity field. Store the computed pressure values in the grid cell struct and enable the pressure render mode in the UI. This allows you to visualize how pressure evolves during the simulation and can help diagnose issues with your projection step.

- Density: Similar to advecting particles, advecting density is a great way to visualize the motion of the fluid. Implement an advection function that updates the density field using the velocity field, and write the resulting density values into the grid cell struct.

- Divergence: After the projection step, the divergence term should be zero. Make sure that the divergence values are zero.

- Check for uninitialized values.
- **For numeric values that your simulation keeps track of (positions, velocities, forces, etc.), use** `double` **and** `Eigen::Vector3d` **instead of** `float` **and** `Eigen::Vector3f`**. The additional precision can have a big impact on your simulation's stability.**
- **REMINDER: Your code will run much faster if you compile in Release mode ;**

### Submission Instructions

Submit your branch of the Github classroom repository to the “Simulation” assignment.

## Example Video

- Reference solution coming soon!

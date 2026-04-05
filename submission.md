# Fluid Sim Submissions

Submissions template for Fluid Sim assignment

## Instructions for Students

1. Place your video submissions in the `student_videos/` folder.
2. Update the links in the "Your Solutions" section with your video filenames.
3. Commit and push your changes.

## Reference Solution Matching Notes

1. The solution implements A MAC grid for stability
2. Depending on how you handle your boundary conditions, your simulation may look different. Especially at the boundary.
3. The magnitude of the forces used will also affect the simulation.

---

## Reference Videos

The vleocity field under the influence of Gravity alone.
- **Simple Fall**  
The vleocity field under the influence of Gravity alone.
  [Watch Reference](example-video/simple_fall.mp4)


***Some example external forces***

- **Swirl**  
  The swirl impulse generates a **rotational velocity** around the center of the domain.  
  - Each grid cell’s velocity is modified based on its position relative to the domain center.  
  - Cells farther from the center have a weaker swirl due to an **exponentially decaying strength** with distance.  


  [Watch Reference](example-video/swirl.mp4)

- **Upward Jet**  
  The upward impulse creates a **rising, fountain-like flow** from the bottom-center of the cube.  
  - Velocity is applied pointing upward and outward, based on each cell’s horizontal distance from the bottom-center.  
  - Strength decays exponentially with horizontal and vertical distance.  

  [Watch Reference](example-video/upward_jet.mp4)

---

## Your Solutions

- **Simple Fall**  
  [Watch Your Video](student_videos/simple_fall_yours.mp4)

- **Swirl**  
  [Watch Your Video](student_videos/swirl_yours.mp4)

- **Upward Jet**  
  [Watch Your Video](student_videos/upward_jet_yours.mp4)
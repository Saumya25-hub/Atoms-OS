# BOS Window Manager V2 Overview

The BOS Window Manager V2 provides the permanent Surface Compositor Foundation for ATOMS OS. It acts as the backbone for all GUI applications, abstracting memory, z-ordering, and rendering logic away from end-user programs.

## Key Subsystems

1. **Surface Engine**: Manages hierarchy and relationships of all drawable entities.
2. **Dirty Region Engine**: Tracks screen invalidations to minimize redraw overhead.
3. **Painter**: Abstract drawing routines that respect clip boundaries.
4. **Compositor**: Blends dirty regions of the surface tree into the backbuffer.
5. **Window Object**: High-level OS constructs mapping surfaces to applications.
6. **Desktop Object**: The root surface node.

## Design Philosophy

- **Zero Memory Leaks**: All resources are explicitly bound to their owners.
- **Modularity**: Subsystems do not bypass each other.
- **Performance**: Framebuffer writes are restricted to dirty areas to achieve 60 FPS under heavy load.

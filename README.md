# TODO List:
- [x] Fill UBO transfer command
- [ ] SDL backend?
	- [ ] SDL Window
	- [ ] SDL Input listeners
	- [ ] SDL Audio
- [ ] CPU Parallel task executions
	- [ ] Task executor
	- [ ] Shader compilation
	- [ ] Pipeline creation
- [x] Common Shaders Definitions
- [x] Commong shaders bindings
- [x] Automatic shader definitions (to progressively complete)
- [x] Shader pragma once
- [x] Dynamic Shaders Definitions (keep a table)
- [x] UpdateContext
- [ ] VkShaderObjectEXT? 
- [ ] Complete Graphics command
    - [x] Bresenham line rasterization
    - [ ] MSAA
    - [ ] FragmentCommand
- [ ] RenderPass-less rendering? 
- [ ] Mesh
	- [ ] Mesh interface
		- [ ] Rigid Mesh
		- [ ] Animated Mesh with skeleton
		- [ ] Terrain
		- [ ] ??? 
	- [ ] .obj loader
	- [ ] Simple mesh generation
		- [ ] Platonic solids
		- [x] Box
		- [x] Sphere
		- [ ] Icosphere
	- [ ] Mesh renderer, Mesh representation
		- [ ] Mesh attribs from reflection
	- [ ] Mesh compression?
	- [ ] Meshlets ? 
- [ ] Scene
- [ ] Mesh Shaders command
- [ ] VkRayTracingKHR
	- [ ] TLAS
	- [ ] SBT
- [x] Modules
- [ ] Multi layer swapchain (for VR)
- [x] Debug buffers
	- [x] GLSL strings
- [x] Input manager
- [ ] Improve CMake
	- [x] AddProject function
	- [ ] Separate list for .exe
	- [x] Option to build .exe
	- [ ] Correctly import Vulkan
- [ ] Better ResourceStateTracker
	- [x] Remake the interface
	- [x] Actually store the resource state in the resource rather than in a separate map?
	- [ ] Correctly track potentially overlapping subresources
		- [x] for Buffers
		- [ ] for Images
- [ ] Re use descriptor sets
	- [ ] set = 0: common set (debug, ...)
	- [ ] set = 1: scene set
	- [ ] set = 2: module set
	- [ ] set = 3: shader set
	- [ ] set = 4: push_descriptor set (shader invocation specific data)
- [ ] Multi window mode (without ImGui)
- [ ] Buffer / Image on recreate instance command policy (copy content)
- [ ] MultiExecutor

- [ ] Find a better project name
- [ ] Rework on thatlib

# Projects Ideas:
- [ ] Basic Renderer
- [ ] Seam Carving
- [ ] Mutiple rigid body simulation
- [ ] Fluid simulation? 
- [ ] Reversed perspective redering
- [ ] Wave Function Collapse
- [ ] PT, LT, BDPT, VCM?
- [ ] SIBR
- [ ] HashCache based field storage + neural recontstruction?
- [ ] ReStiR
- [ ] Radiance guiding

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
	- [x] Mesh Shaders command
    - [ ] MSAA
    - [ ] FragmentCommand
- [ ] RenderPass-less rendering? 
- [ ] Mesh
	- [x] Mesh interface
		- [x] Rigid Mesh
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
- [x] Better ResourceStateTracker
	- [x] Remake the interface
	- [x] Actually store the resource state in the resource rather than in a separate map?
	- [x] Correctly track potentially overlapping subresources
		- [x] for Buffers
		- [x] for Images
- [x] Fix synchronization validation errors
- [x] Re use descriptor sets
	- [x] set = 0: common set (debug, ...), managed by the executor
	- [x] set = 1: scene set
	- [ ] set = 2: module set
	- [x] set = 3: shader set
	- [x] set = 4: push_descriptor set (shader invocation specific data)
- [ ] CmdBuffer bindings (pipeline + desc sets)
	- [ ] Bind pipeline -> reset desc bindings?
- [x] DescSetLayout cache managed by the application
- [ ] Accumulate all descriptor updates in the UpdateContext
- [ ] Multi window mode (independent of ImGui)
- [ ] Buffer / Image on recreate instance command policy (copy content)
- [ ] MultiExecutor
- [ ] Cmake: Make and Optimized mode (debug info for my code, other libs ands stl linked in release with IDL = 0)

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


# Loose Ideas:
There should be two types of resources, synch ones and asynch ones.
- Synch resources (curent implementation) are intended to be "script resources", in a limited number, updated by the main thread, always loaded and ready to use
- Asynch resources are intended to be meshes, textures, etc, in a large number, managed (loaded, updated) by asynch threads and can be not avaible yet, partially available, or complitely available

# Resources management / Execution refactor
Objectives:
- [ ] Synchronize multiple times the same resources for the same or different usages (refactor of SynchHelper)
- [x] Have more dynamic descriptor sets (adding or removing descriptors "on the fly")
- [x] Resource management: have resources be managed by their owner ifp, else by the script manager, instead of only by the executor
- [ ] Execute host tasks in parallel (such as shader compilation, resources loading)
- [ ] Load scene (mesh, textures) asynch
- [x] Refactor the Executor (and linearExecutor): issue an ExecThread to record commands in
- [ ] Add a transfer queue to the LinearExecutor to load assets
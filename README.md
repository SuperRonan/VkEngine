# TODO List:
- [x] Fill UBO transfer command
- [x] SDL backend?
	- [x] SDL Window
	- [x] SDL Input listeners
	- [ ] SDL Audio
- [x] CPU Parallel task executions
	- [x] Task executor
	- [x] Shader compilation
	- [x] Pipeline creation
- [x] Common Shaders Definitions
- [x] Common shaders bindings
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
	- [x] .obj loader
	- [ ] Simple mesh generation
		- [ ] Platonic solids
		- [x] Box
		- [x] Sphere
		- [ ] Icosphere
	- [ ] Mesh renderer, Mesh representation
		- [ ] Mesh attribs from reflection
	- [ ] Mesh compression?
	- [ ] Meshlets? 
	- [ ] Separate positions from other vertex data?
- [x] Scene
- [ ] VkRayTracingKHR
	- [x] TLAS
	- [ ] SBT
- [x] Modules
- [ ] Multi layer swapchain (for VR)
- [x] Debug buffers
	- [x] GLSL strings
- [x] Input manager
- [ ] Improve CMake
	- [x] AddProject function
	- [x] Separate list for .exe
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
- [x] Accumulate all descriptor updates in the UpdateContext
- [ ] Multi window mode (independent of ImGui)
- [x] Buffer / Image on recreate instance command policy (copy content)
- [ ] MultiExecutor
- [ ] Cmake: Make and Optimized mode (debug info for my code, other libs ands stl linked in release with IDL = 0)
- [x] Window:
	- [x] Correct HDR support (also ImGui)
	- [x] Fullscreen
	- [x] Resized by the application
- [ ] UpdateResources takes a priority as parameter, so for instance shaders that will not be used can still be updated, but with a lower priority
- [x] Add a mode to use the GENERAL image layout instead of the specialized layout and measure the performance of doing so 
- [x] Refactor ResourceBinding and remove Resource 
- [x] Remove ext features from the device requested feature chain when the extension is not used (it now generates a validation error)
- [ ] Separate DescriptorSet and DescriptorPool
	- [ ] Use one pool for many descriptor sets (for model resources)

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
	- [x] Simple Resource usage list with one usage per resources (works for now)
- [x] Have more dynamic descriptor sets (adding or removing descriptors "on the fly")
- [x] Resource management: have resources be managed by their owner ifp, else by the script manager, instead of only by the executor
- [x] Execute host tasks in parallel (such as shader compilation, resources loading)
- [ ] Load scene (mesh, textures) asynch
	- [x] Asynch Textures
	- [ ] Asynch Mesh
	- [ ] Asynch Mesh group
- [x] Refactor the Executor (and linearExecutor): issue an ExecThread to record commands in
- [ ] Add a transfer queue to the LinearExecutor to load assets
- [x] Refactor command execition:
	- [x] Command execution issue an execution object declaring the resources on which it will interact and how, synchronization of said resources will be done externaly by the executor. This allows for ASAP synch, rather than the current mandatory ALAP
	- [ ] Auto Build an execution graph from these execution objects (nodes) and their resources (edges)
- [x] Explicitely instancify Resources
- [x] ExecutionNodes are Command's instances, as such, they should only reference resources instances. 
- [ ] Have a separate render thread
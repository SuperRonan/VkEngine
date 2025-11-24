# VkEngine
- Slang and GLSL shader development platform.
- Shader debbugging utilities.
- Automatic shader reloading.
- Automatic GPU synchronization.
- HDR windows management.
- Multi-threaded background tasks.
- ImGui integration to easily tune parameters.
- Rendering utilities support (Scene, Cameras, Light, Mesh, Materials, ...).

## Installation
To download the engine: 
```bash
git clone https://github.com/SuperRonan/VkEngine.git --recurse-submodules
```
Use this command to pull the submodules if you forgot to do so with the first command: 
```bash
$ git submodule update --init --recursive
```
VkEngine also requires the [VulkanSDK](https://vulkan.lunarg.com/) to be installed. **A recent version containing the Slang libraries is required**.
You can then use CMake to build the engine.

Supported platforms:
- Windows: Supported
- Linux: Not Supported (coming soon)
- MacOs: Not Supported

## Renderer
- Interactive rendering engine.
- Basic scene edition with ImGui (place objects, change some properties, ...)
- Light transport algorithms (Path Tracer, Light tracer, Bidirectional Path Tracer).
	- Spectral rendering options.
- More rendering experimentations to come...
![sponza-sunlight-2](https://github.com/user-attachments/assets/be6c6dd7-22ac-46cc-81ee-96e1852cb759)
![sponza-sunlight](https://github.com/user-attachments/assets/fa7f7bcb-c3b2-4355-9ef3-f613fd3bb788)
![metallic](https://github.com/user-attachments/assets/a8fb1576-d939-4c77-b608-0da715b1e70b)
![caustics](https://github.com/user-attachments/assets/1e55a2a0-807a-4fff-b992-662459f4a8e1)
![caustics-rgb](https://github.com/user-attachments/assets/45a17ba9-13ef-495c-bf51-e6a3cdcd402c)
![caustics-dof](https://github.com/user-attachments/assets/aa956783-ff06-4bbf-96bd-5ea61b165496)
![caustics-dof-2](https://github.com/user-attachments/assets/1e161f62-853e-426b-937e-e8c8be36e462)
![caustics-dispersion](https://github.com/user-attachments/assets/e095bed2-ac00-44f7-9f15-1bae4835af53)
![caustics-gem](https://github.com/user-attachments/assets/34aed66d-7c92-4db0-995d-fc7a094ab265)


## BSDF viewer
Interactive 3D BSDF visualizer. 
- Compute statistics (correlation, variance, integral, ...) of different spherical functions.
- Customize the viewed BSDFs by directly changing the shader source code.
![bsdf](https://github.com/user-attachments/assets/ea6ec614-c991-4487-9518-33012722baab)

## Shader debugging
It is often tidious to debug shaders.
The engine implements shader debugging utilities to ease this process, without using external tools.

https://github.com/user-attachments/assets/4c34aabf-2dd6-481f-a384-4ba254ef5b6e

It is possible to print values (builtin types, vectors, matrices, arrays of builtin, string litterals (in GLSL, wip for Slang) or any type conforming to `IPrintable`) for quick inspection.
We can place the printing cursor in screen space, or in 3D space to show when the thread emitting the information is.
It is also possible to print lines between two points (to show a traced path for example).

## Command line arguments
```
-h, --help                    shows help message and exits
-v, --version                 prints version information and exits
--name                        Name of the Application
--validation                  Enable Vulkan Validation Layers [nargs=0..1] [default: 0]
--name_vk_objects             Name Vulkan Objects [nargs=0..1] [default: 1]
--cmd_labels                  Set Labels in the CommandBuffer [nargs=0..1] [default: 1]
--helper_threads              Set the number of helper threads, 'all' to use all available threads, 'none' to run single thread, -n to use all threads minus n [nargs=0..1] [default: "all"]
--gpu                         Select the index of the gpu to use [nargs=0..1] [default: -1]
--image_layout                Select which image layout to use, possible values are: 'specific', 'general', 'auto' or a bit mask per usage (0 for specfic, 1 for general) [nargs=0..1] [default: "specific"]
--verbosity                   Set the console verbosity level (int) [nargs=0..1] [default: 2]
--dump_shader_source          Enable shaders source dump in the gen folder [nargs=0..1] [default: 0]
--dump_preprocessed_shader    Enable preprocessed shaders source dump in the gen folder [nargs=0..1] [default: 0]
--dump_spv                    Enable shaders SPIR-V binary dump in the gen folder [nargs=0..1] [default: 0]
--dump_slang_to_glsl          Compile Slang Shaders to GLSL and dump in the gen folder [nargs=0..1] [default: 0]
--shaderc_optimization_level  Set ShaderC optimization level [nargs=0..1] [default: 2]
--slang_optimization_level    Set Slang optimization level [nargs=0..1] [default: 3]
--resolution                  Set the resolution of the main window [nargs: 2]
--imgui_docking               Force the ImGui Docking feature (0 or 1)
--imgui_multi_viewport        Force the ImGui Multi Viewport feature (0 or 1)
```

## TODO List:
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
- [x] VkRayTracingKHR
	- [x] TLAS
	- [x] SBT
- [x] Modules
- [ ] Multi layer swapchain (for VR)
- [x] Debug buffers
	- [x] GLSL strings
- [x] Input manager
- [ ] Improve CMake
	- [x] AddProject function
	- [x] Separate list for .exe
	- [x] Option to build .exe
	- [x] FastDebug build (Same as Debug, with IDL=0)
	- [x] Correctly import Vulkan
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
	- [x] set = 2: module set
	- [x] set = 3: shader set
	- [x] set = 4: push_descriptor set (shader invocation specific data)
- [ ] CmdBuffer bindings (pipeline + desc sets)
	- [ ] Bind pipeline -> reset desc bindings?
- [x] DescSetLayout cache managed by the application
- [x] Accumulate all descriptor updates in the UpdateContext
- [ ] Multi window mode (independent of ImGui)
- [x] Buffer / Image on recreate instance command policy (copy content)
- [ ] MultiExecutor
- [x] Cmake: Make and Optimized mode (debug info for my code, other libs ands stl linked in release with IDL = 0)
- [x] Window:
	- [x] Correct HDR support (also ImGui)
	- [x] Fullscreen
	- [x] Resized by the application
- [ ] UpdateResources takes a priority as parameter, so for instance shaders that will not be used can still be updated, but with a lower priority
	- For now, AsynchTasks are executed asynch, but are joined synch, which allows them to have side effect (store their result in the object emmiting the task). It should be forbiden, as it might create bugs, especially with an asynch joining of tasks. 
- [x] InstanceHolders (aka Descriptors) have an option to not hold and instance (mostly to free memory for Images and Buffers)
- [x] Add a mode to use the GENERAL image layout instead of the specialized layout and measure the performance of doing so 
- [x] Refactor ResourceBinding and remove Resource 
- [x] Remove ext features from the device requested feature chain when the extension is not used (it now generates a validation error)
- [ ] Separate DescriptorSet and DescriptorPool
	- [ ] Use one pool for many descriptor sets (for model resources)
- [x] Add helper types StringVector, AnyVector, ...
- [ ] Refactor UploadQueue, AsynchUpload, ... to avoid using individual small allocations
- [ ] Make a common interface render pass

- [ ] Find a better project name
- [ ] Rework on thatlib

## Projects Ideas:
- [x] Basic Renderer
- [ ] Seam Carving
- [ ] Mutiple rigid body simulation
- [ ] Fluid simulation? 
- [x] Reversed perspective redering
- [ ] Wave Function Collapse
- [x] PT, LT, BDPT, VCM?
- [ ] SIBR
- [ ] HashCache based field storage + neural recontstruction?
- [ ] ReStiR
- [ ] Radiance guiding


## Loose Ideas:
October 2023:
There should be two types of resources, synch ones and asynch ones.
- Synch resources (curent implementation) are intended to be "script resources", in a limited number, updated by the main thread, always loaded and ready to use
- Asynch resources are intended to be meshes, textures, etc, in a large number, managed (loaded, updated) by asynch threads and can be not avaible yet, partially available, or complitely available
April 2024: 
The distinction has kind of been implemented now.
Problem: automatic synchronization of all the scene resources is expesive. But all these resources are synched the "same" way, the synchronization work can be done once and shared to all these resources.
Maybe create a distinction between two synch regimes: Auto and Manual. 

## Resources management / Execution refactor
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

## Big problem with DynamicValue
In its current implementation, DynamicValue is not thread safe and may lead to crashes!
DynValueInstance::value() is marked as const, which would be thread safe, but is it not because of the mutable cached value.
The problem is currently 99% under control (it has probably never occured while using the engine, but only as a thought experiment and in a stress test designed for it)
So its resolution is necessary, but not urgent. 
I see 3 possible solutions:
- Add a (shared_)mutex to DynValueInstance to lock the evaluation (common solution to keep mutable in a multi threaded context). This might not be enough since we would need to keep the lock when viewing the cached value with a reference. Maybe create a wrapped type like SharedLockedView? 
- Remove the mutable cached value from DynValueInstance (maybe move it to DynValue)
- Rethink the whole DynValue architechture
	- Evalute DV's with an EvaluationContext, which would contain a index of the cycle
	- Each DV would keep a cached value, assiciated with a cycle index, and shared_mutex
	- Uppon evaluation, DV's would be recursivly evaluated with a ctx, only evaluating when the ctx cycle index is greater than the cached value's one. The mutex would be locked during the evaluation 
	- So each DV would idealy be evaluated only once
	- Maybe keep not just one cached value, bu a small buffer of n values to keep a history, a be able to access previous values (allowing to evaluate the next frame DV's while reading their value for the previous frame)

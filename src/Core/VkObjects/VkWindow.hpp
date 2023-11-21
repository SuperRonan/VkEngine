#pragma once

#include <Core/App/VkApplication.hpp>

#include <Core/IO/ImGuiUtils.hpp>

#include <set>
#include <string>
#include <vector>
#include "ImageView.hpp"
#include "Semaphore.hpp"
#include "Fence.hpp"
#include <chrono>
#include "Surface.hpp"
#include "Swapchain.hpp"

namespace vkl
{

	class VkWindow : public VkObject
	{
	public:

		using ivec2 = glm::ivec2;
		using float2 = glm::vec2;

		enum class Mode
		{
			Windowed = 0,
			WindowedFullscreen = 1,
			Fullscreen = 2,
			ExclusiveFullscreen = 3,
			MAX_ENUM,
		};

		struct CreateInfo
		{	
			VkApplication* app = nullptr;
			std::set<uint32_t> queue_families_indices = {};

			DynamicValue<VkPresentModeKHR> target_present_mode = VK_PRESENT_MODE_FIFO_KHR;

			std::string name = {};
			Mode mode = Mode::Windowed;
			uint32_t w, h;
			int resizeable = false;
		};

	protected:

		uint32_t _width, _height;
		Mode _window_mode = Mode::MAX_ENUM;
		GLFWwindow* _window;

		int _window_pos_x, _window_pos_y;
		int _latest_windowed_width, _latest_windowed_height;

		struct DetailedMonitor
		{
			GLFWmonitor * handle;
			
			std::string name;
			ivec2 position;
			ivec2 pixel_size;
			ivec2 physical_size;
			float2 scale;
			GLFWvidmode default_vidmode;
			GLFWvidmode current_vidmode;
			std::vector<GLFWvidmode> available_video_modes;
			const GLFWvidmode * video_mode;

			float gamma;

			void query();

			void setGamma(float gamma);
		};

		int _desired_resolution[2];

		std::vector<DetailedMonitor> _monitors = {};
		size_t _selected_monitor_index = 0;

		std::shared_ptr<Surface> _surface = nullptr;
		std::shared_ptr<Swapchain> _swapchain = nullptr;
		
		
		DynamicValue<VkExtent3D> _dynamic_extent;
		std::set<uint32_t> _queues_families_indices;

		bool _glfw_resized = false;
		bool _gui_resized = false;

		size_t _current_frame = 0;
		// Of size swapchain (One per swap image)
		//std::vector<std::shared_ptr<Fence>> _image_in_flight_fence;

		ImGuiListSelection _gui_window_mode;
		Mode _desired_window_mode;

		DynamicValue<VkPresentModeKHR> _target_present_mode;
		ImGuiListSelection _gui_present_modes;
		DynamicValue<VkSurfaceFormatKHR> _target_format = VkSurfaceFormatKHR{.format = VK_FORMAT_MAX_ENUM, .colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR};
		ImGuiListSelection _gui_formats;

		struct FrameInfo
		{
			uint32_t index;
		};

		FrameInfo _current_frame_info;

		std::chrono::time_point<std::chrono::system_clock> _present_time_point = std::chrono::system_clock::now();
		size_t _present_frame = 0;

		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

		void initGLFW(std::string const& name, int resizeable);

		void createSwapchain();

		virtual void init(CreateInfo const& ci);

		

		void initSwapchain();

		void cleanupSwapchain();

		virtual void cleanup();


		void setupGuiObjects();

		void saveWindowedAttributes();


	public:


		VkWindow(CreateInfo const& ci);

		// The window is not copyable because of the glfw callback that need the same pointer (though it could be handled)
		VkWindow(VkWindow const&) = delete;

		VkWindow(VkWindow&&) = delete;

		VkWindow& operator=(VkWindow const&) = delete;

		VkWindow& operator=(VkWindow&&) = delete;

		virtual ~VkWindow();

		bool shouldClose()const;

		void pollEvents();

		void updateWindowIFP();

		void setSize(uint32_t w, uint32_t h);

		std::shared_ptr<Image> image(uint32_t index);

		std::shared_ptr<ImageView> view(uint32_t index);

		constexpr std::vector<std::shared_ptr<ImageView>> const& views() const
		{
			return _swapchain->instance()->views();
		}

		DynamicValue<VkSurfaceFormatKHR> surfaceFormat()
		{
			return _target_format;
		}

		DynamicValue<VkFormat> format()const;

		size_t swapchainSize()const;

		DynamicValue<VkExtent3D> extent3D() const
		{
			return _dynamic_extent;
		}

		DynamicValue<VkExtent2D> extent2D() const
		{
			return _swapchain->extent();
		}

		constexpr operator GLFWwindow* ()const
		{
			return _window;
		}

		constexpr GLFWwindow* handle()const
		{
			return _window;
		}

		const std::shared_ptr<Swapchain>& swapchain() const
		{
			return _swapchain;
		}

		struct AquireResult
		{
			VkBool32 success;
			uint32_t swap_index;
			//uint32_t in_flight_index;
			//std::shared_ptr<Semaphore> semaphore;
			//std::shared_ptr<Fence> fence;

			AquireResult();

			AquireResult(uint32_t swap_index);
		};

		AquireResult aquireNextImage(std::shared_ptr<Semaphore> semaphore_to_signal, std::shared_ptr<Fence> fence_to_signal);

		void present(uint32_t num_semaphores, VkSemaphore* semaphores);

		bool updateResources(UpdateContext & ctx);

		void declareImGui();

	};

}
#pragma once

#include <Core/App/VkApplication.hpp>

#include <Core/IO/ImGuiUtils.hpp>
#include <Core/IO/GuiContext.hpp>

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
			std::string name = {};

			std::set<uint32_t> queue_families_indices = {};
			
			glm::ivec2 resolution = {};
			Dyn<glm::ivec2> dynamic_resolution = {};
			
			VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
			Dyn<VkPresentModeKHR> dynamic_present_mode = {};
			
			//VkSurfaceFormatKHR target_format = VkSurfaceFormatKHR{
			//	.format = VK_FORMAT_MAX_ENUM,
			//	.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR,
			//};
			Dyn<VkSurfaceFormatKHR> dynamic_target_format = {};

			Mode mode = Mode::Windowed;
			
			int resizeable = false;
		};
		using CI = CreateInfo;

	protected:

		uint32_t _width, _height;
		Mode _window_mode = Mode::MAX_ENUM;
		SDL_Window* _window;

		bool _resizeable = false;

		int _window_pos_x, _window_pos_y;
		int _latest_windowed_width, _latest_windowed_height;

		int _desired_monitor_index = 0;
		int _desired_display_mode_index = 0;

		std::shared_ptr<Surface> _surface = nullptr;
		std::shared_ptr<Swapchain> _swapchain = nullptr;
		

		Dyn<glm::ivec2> _extern_resolution = {};
		glm::ivec2 _desired_resolution = {};
		Dyn<VkExtent3D> _dynamic_extent = {};
		

		std::set<uint32_t> _queues_families_indices;

		bool _sdl_resized = false;
		bool _gui_resized = false;

		bool _should_close = false;

		size_t _current_frame = 0;
		// Of size swapchain (One per swap image)
		//std::vector<std::shared_ptr<Fence>> _image_in_flight_fence;

		ImGuiListSelection _gui_window_mode;
		Mode _desired_window_mode;

		Dyn<VkPresentModeKHR> _extern_present_mode = {};
		VkPresentModeKHR _target_present_mode = VK_PRESENT_MODE_FIFO_KHR;
		ImGuiListSelection _gui_present_modes;

		Dyn<VkSurfaceFormatKHR> _extern_target_format = {};
		VkSurfaceFormatKHR _target_format = VkSurfaceFormatKHR{
			.format = VK_FORMAT_MAX_ENUM,
			.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR,
		};
		ImGuiListSelection _gui_formats;

		struct FrameInfo
		{
			uint32_t index;
		};

		FrameInfo _current_frame_info;

		std::chrono::time_point<std::chrono::system_clock> _present_time_point = std::chrono::system_clock::now();
		size_t _present_frame = 0;

		static void frameBufferResizeCallback(SDL_Window* window, int width, int height);

		void initSDL();

		void preventFlickerWhenFullscreen();

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

		bool shouldClose()const
		{
			return _should_close;
		}

		bool eventIsRelevent(SDL_Event const& event) const;

		bool processEventCheckRelevent(SDL_Event const& event);

		void processEventAssumeRelevent(SDL_Event const& event);

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
			return &_target_format;
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

		constexpr operator SDL_Window* ()const
		{
			return _window;
		}

		constexpr SDL_Window* handle()const
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

			AquireResult();

			AquireResult(uint32_t swap_index);
		};

		AquireResult aquireNextImage(std::shared_ptr<Semaphore> semaphore_to_signal, std::shared_ptr<Fence> fence_to_signal);


		//void present(uint32_t num_semaphores, VkSemaphore* semaphores, VkFence fence = VK_NULL_HANDLE);

		void notifyPresentResult(VkResult res);

		bool updateResources(UpdateContext & ctx);

		void declareGui(GuiContext & ctx);

	};

}
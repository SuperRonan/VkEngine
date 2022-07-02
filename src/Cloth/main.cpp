
#include <Core/VkApplication.hpp>
#include <Core/VkWindow.hpp>
#include <Core/ImageAndView.hpp>
#include <Core/Shader.hpp>
#include <Core/Camera2D.hpp>
#include <Core/MouseHandler.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Core/Buffer.hpp>
#include <Core/Sampler.hpp>
#include <Core/Pipeline.hpp>
#include <Core/PipelineLayout.hpp>

#include <iostream>
#include <chrono>
#include <random>
#include <memory>

namespace vkl
{
	class SpringMass2D : public VkObject
	{
		struct Vertex
		{
			glm::vec3 position;
			glm::vec3 speed;
			glm::vec3 normal;
			float mass;
		};

	protected:

		
		VkExtent2D _extent;
		std::vector<Buffer> _buffers;
		Buffer _index_buffer;


	public:

		SpringMass2D(VkApplication * app, VkExtent2D extent):
			VkObject(app),
			_extent(extent),
			_buffers(2)
		{
			size_t elems = _extent.width * _extent.height;
			size_t buffer_size = elems * sizeof(Vertex);
			VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			for (size_t i = 0; i < _buffers.size(); ++i)
			{
				_buffers[i] = Buffer(_app);
				_buffers[i].createBuffer(buffer_size, usage);
			}

			std::vector<Vertex> vertices(elems);
			glm::vec3 base(0, 0, 0);
			glm::vec3 offset(1, 1, 1);
			glm::vec3 pos(0, 0, 0);

			size_t tiles = (_extent.width - 1) * (_extent.height - 1);
			std::vector<uint32_t> indices(tiles * 2 * 3);

			_index_buffer = Buffer(_app);
			_index_buffer.createBuffer(tiles * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			
			for (size_t y = 0; y < _extent.height; ++y)
			{
				pos.y = float(y) / float(_extent.height);
				for (size_t x = 0; x < _extent.width; ++x)
				{
					pos.x = float(x) / float(_extent.width);
					Vertex v{
						.position = pos,
						.speed = glm::vec3(0),
						.normal = glm::vec3(0, 0, 1),
						.mass = 1.0f,
					};
					size_t index = x + y * _extent.width;
					vertices[index] = v;
				}
			}

			for (size_t y = 0; y < _extent.height - 1; ++y)
			{
				size_t _tile = y * (_extent.width - 1);
				for (size_t x = 0; x < _extent.width - 1; ++x)
				{
					size_t tile = _tile + x;
					size_t index = tile * 2 * 3;
					indices[index + 0] = y * _extent.width + x;
					indices[index + 1] = y * _extent.width + x + 1;
					indices[index + 2] = (y + 1) * _extent.width + x;
					indices[index + 3] = y * _extent.width + x + 1;
					indices[index + 4] = (y + 1) * _extent.width + x;
					indices[index + 5] = (y + 1) * _extent.width + x + 1;
				}
			}

			auto staging = _buffers[0].copyToStaging(vertices.data(), buffer_size);
			auto index_staging = _index_buffer.copyToStaging(indices.data(), indices.size() * sizeof(uint32_t));

			VkCommandBuffer cmd = _app->beginSingleTimeCommand(_app->pools().graphics);
			{
				_buffers[0].recordCopyStagingToBuffer(cmd, staging);
				_index_buffer.recordCopyStagingToBuffer(cmd, staging);
			}
			_app->endSingleTimeCommandAndWait(cmd, _app->queues().graphics, _app->pools().graphics);

			_app->stagingPool().releaseStagingBuffer(staging);
			_app->stagingPool().releaseStagingBuffer(index_staging);
		}

		constexpr const auto& buffers()const
		{
			return _buffers;
		}

		constexpr const auto& indexBuffer()const
		{
			return _index_buffer;
		}

	};

	class ClothApp : public VkApplication
	{

	protected:

		std::shared_ptr<VkWindow> _window;

		std::shared_ptr<SpringMass2D> _cloth;

		std::shared_ptr<Pipeline> _update_pipeline;

		void createDescriptorLayout()
		{
			{
				VkDescriptorSetLayoutBinding prev{
					.binding = 0,
					
				};
			}
		}

		void createUpdatePipeline()
		{
			//std::filesystem::path shader_path = std::string(ENGINE_SRC_PATH) + "/src/Cloth/updateSpringMass2D.comp";
			//std::shared_ptr<Shader> update_shader = std::make_shared<Shader>(this, shader_path, VK_SHADER_STAGE_COMPUTE_BIT);
			//std::shared_ptr<ComputeProgram> update_program = std::make_shared<ComputeProgram>(update_shader);
			//_update_pipeline = std::make_shared<Pipeline>(update_program);
		}

		void createRenderPipeline()
		{

		}

	public:

		ClothApp(bool validation = false):
			VkApplication("Cloth", validation)
		{
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.in_flight_size = 2,
				.target_present_mode = VK_PRESENT_MODE_FIFO_KHR,
				.name = "Cloth",
				.w = 2048,
				.h = 1024,
				.resizeable = GLFW_FALSE,
			};

			_window = std::make_shared<VkWindow>(window_ci);

			_cloth = std::make_shared<SpringMass2D>(this, VkExtent2D{ 100, 100 });

			createUpdatePipeline();
		}


		virtual void run()
		{
			while (!_window->shouldClose())
			{
				_window->pollEvents();

			}
		}
	};

}

void main()
{
	try
	{
		vkl::ClothApp app(true);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
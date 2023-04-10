#pragma once

#include "DeviceCommand.hpp"
#include <utility>
#include <cassert>

namespace vkl
{
	class PushConstant
	{
	protected:

		std::vector<uint8_t> _data = {};

	public:

		constexpr PushConstant() = default;

		template<class T>
		PushConstant(T&& t) :
			_data(sizeof(T))
		{
			std::memcpy(_data.data(), &t, sizeof(T));
		}

		template <class T>
		PushConstant& operator=(T&& t)
		{
			_data.resize(sizeof(T));
			std::memcpy(_data.data(), &t, sizeof(T));
			return *this;
		}

		PushConstant(PushConstant const& o):
			_data(o._data)
		{}

		PushConstant(PushConstant && o) noexcept:
			_data(std::move(o._data))
		{}

		PushConstant& operator=(PushConstant const& o)
		{
			_data = o._data;
			return *this;
		}

		PushConstant& operator=(PushConstant&& o) noexcept
		{
			_data = std::move(o._data);
			return *this;
		}

		const uint8_t* data()const
		{
			return _data.data();
		}

		operator const void* ()const
		{
			return data();
		}

		size_t size()const
		{
			return _data.size();
		}

		operator bool()const
		{
			return !_data.empty();
		}
	};

	struct ShaderBindingDescription
	{
		std::shared_ptr<Buffer> buffer = nullptr;
		std::shared_ptr<ImageView> view = nullptr;
		std::shared_ptr<Sampler> sampler = nullptr;
		std::string name = {};
		uint32_t set = uint32_t(0);
		uint32_t binding = uint32_t(-1);
	};
	
	using Binding = ShaderBindingDescription;

	class ResourceBinding
	{
	public:

	protected:
		Resource _resource = {};
		std::shared_ptr<Sampler> _sampler = {};
		uint32_t _binding = uint32_t(-1);
		uint32_t _set = 0;

		uint32_t _resolved_set = 0, _resolved_binding = uint32_t(-1);
		std::string _name = "";
		VkDescriptorType _type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		bool _updated = false;

	public:

		ResourceBinding(ShaderBindingDescription const& desc);

		constexpr void resolve(uint32_t s, uint32_t b)
		{
			_resolved_set = s;
			_resolved_binding = b;
		}

		constexpr bool isResolved()const
		{
			return _resolved_binding != uint32_t(-1);
		}

		constexpr void setBinding(uint32_t s, uint32_t b)
		{
			_set = s;
			_binding = b;
		}

		constexpr bool resolveWithName()const
		{
			return _binding == uint32_t(-1);
		}

		constexpr const std::string& name()const
		{
			return _name;
		}

		template <class StringLike>
		constexpr void setName(StringLike && name)
		{
			_name = std::forward<StringLike>(name);
		}

		constexpr auto type()const
		{
			return _type;
		}

		constexpr void setType(VkDescriptorType type)
		{
			_type = type;
		}

		constexpr bool isBuffer()const
		{
			return
				_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
				_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		constexpr bool isImage()const
		{
			return
				_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
				_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
				_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		constexpr bool isSampler()
		{
			return
				_type == VK_DESCRIPTOR_TYPE_SAMPLER ||
				_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		constexpr auto& buffer()
		{
			return _resource._buffer;
		}

		constexpr auto& image()
		{
			return _resource._image;
		}

		constexpr auto& sampler()
		{
			return _sampler;
		}

		constexpr VkDescriptorType vkType()const
		{
			return (VkDescriptorType)_type;
		}

		constexpr auto set()const
		{
			return _set;
		}

		constexpr auto binding()const
		{
			return _binding;
		}

		constexpr auto resolvedBinding()const
		{
			return _resolved_binding;
		}
		
		constexpr auto resolvedSet()const
		{
			return _resolved_set;
		}

		constexpr const auto& resource()const
		{
			return _resource;
		}

		constexpr auto& resource()
		{
			return _resource;
		}

		constexpr void release()
		{
			_resolved_binding = uint32_t(-1);
		}

		constexpr bool updated()const
		{
			return _updated;
		}

		constexpr void setUpdateStatus(bool status)
		{
			_updated = status;
		}

	};

	class DescriptorSetsManager : public VkObject
	{
	protected:
		
		std::shared_ptr<Program> _prog;
		std::vector<std::shared_ptr<DescriptorPool>> _desc_pools;
		std::vector<std::shared_ptr<DescriptorSet>> _desc_sets;
		std::vector<ResourceBinding> _bindings;
		std::vector<Resource> _resources;
	
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<ShaderBindingDescription> bindings = {};
		};
		using CI = CreateInfo;

		DescriptorSetsManager(CreateInfo const& ci):
			VkObject(ci.app, ci.name),
			_bindings(ci.bindings.cbegin(), ci.bindings.cend())
		{

		}
		
		virtual ~DescriptorSetsManager() override;

		void setProgram(std::shared_ptr<Program> const& prog)
		{
			_prog = prog;
		}

		void invalidateDescriptorSets();

		void allocateDescriptorSets();

		void resolveBindings();

		void writeDescriptorSets();

		void recordBindings(CommandBuffer& cmd, VkPipelineBindPoint binding);

		void recordInputSynchronization(InputSynchronizationHelper & synch);

	};

	class ShaderCommand : public DeviceCommand
	{
	protected:

		DescriptorSetsManager _sets;

		std::shared_ptr<Pipeline> _pipeline;

		PushConstant _pc;

	public:

		template <typename StringLike = std::string>
		ShaderCommand(VkApplication* app, StringLike&& name, std::vector<ShaderBindingDescription> const& bindings) :
			DeviceCommand(app, std::forward<StringLike>(name)),
			_sets(DescriptorSetsManager::CI{
				.app = app,
				.name = this->name() + ".descs",
				.bindings = bindings,
			})
		{

		}

		virtual ~ShaderCommand() override = default;

		virtual void recordBindings(CommandBuffer& cmd, ExecutionContext& context);

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context) = 0;

		template<typename T>
		void setPushConstantsData(T && t)
		{
			_pc = t;
		}

		virtual void init() override = 0;

		virtual void execute(ExecutionContext& context) override = 0;

		virtual bool updateResources() override;

	};
}
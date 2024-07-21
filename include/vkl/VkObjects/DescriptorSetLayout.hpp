#pragma once

#include <vkl/App/VkApplication.hpp>
#include <concepts>
#include <vkl/VkObjects/AbstractInstance.hpp>
#include <vkl/VkObjects/Sampler.hpp>
#include <vkl/Execution/UpdateContext.hpp>

namespace vkl
{
	class DescriptorSetLayoutInstance : public AbstractInstance
	{
	public:

		struct BindingMeta
		{
			std::string name = {};
			VkAccessFlags2 access = VK_ACCESS_NONE_KHR;
			VkImageLayout layout = VK_IMAGE_LAYOUT_MAX_ENUM;
			VkFlags64 usage = 0;
		};

		VkDescriptorSetLayout _handle = VK_NULL_HANDLE;

		// Sorted by binding index
		MyVector<VkDescriptorSetLayoutBinding> _bindings;
		MyVector<BindingMeta> _metas;

		VkDescriptorSetLayoutCreateFlags _flags = 0;
		VkDescriptorBindingFlags _binding_flags = 0;

	protected:

		struct CreateInfo;
		void create(CreateInfo const& ci);

		void setVkName();

		void destroy();


	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkDescriptorSetLayoutCreateFlags flags = 0;
			MyVector<VkDescriptorSetLayoutBinding> vk_bindings;
			MyVector<BindingMeta> metas;
			VkDescriptorBindingFlags binding_flags = 0;
		};
		using CI = CreateInfo;

		DescriptorSetLayoutInstance(CreateInfo const& ci);
		DescriptorSetLayoutInstance(CreateInfo && ci);

		virtual ~DescriptorSetLayoutInstance() override;

		constexpr decltype(auto) handle()const
		{
			return _handle;
		}

		constexpr operator VkDescriptorSetLayout()const
		{
			return handle();
		}

		constexpr VkDescriptorSetLayout descriptorSetLayout()const
		{
			return handle();
		}
		
		constexpr bool empty()const
		{
			return _bindings.empty();
		}

		constexpr const auto& bindings()const
		{
			return _bindings;
		}

		constexpr const auto& metas()const
		{
			return _metas;
		}

		constexpr VkDescriptorSetLayoutCreateFlags flags()const
		{
			return _flags;
		}

		constexpr VkDescriptorBindingFlags bindingFlags()const
		{
			return _binding_flags;
		}
	};

	class DescriptorSetLayout : public InstanceHolder<DescriptorSetLayoutInstance>
	{
	public:

		using ParentType = InstanceHolder<DescriptorSetLayoutInstance>;

		struct Binding
		{
			std::string name = {};
			uint32_t binding = 0;
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			Dyn<uint32_t> count = {};
			VkShaderStageFlags stages = 0;
			//std::vector<std::shared_ptr<Sampler>> immutable_samplers = {}; // TODO
			VkAccessFlagBits2 access = VK_ACCESS_2_NONE;
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkFlags usage = 0;

			VkDescriptorSetLayoutBinding getVkBinding()const
			{
				return VkDescriptorSetLayoutBinding{
					.binding = binding,
					.descriptorType = type,
					.descriptorCount = count.valueOr(1),
					.stageFlags = stages,
					.pImmutableSamplers = nullptr,
				};
			}

			DescriptorSetLayoutInstance::BindingMeta getMeta() const
			{
				DescriptorSetLayoutInstance::BindingMeta res{
					.name = name,
					.access = access,
					.layout = layout,
					.usage = usage,
				};
				return res;
			}
		};

	protected:

		bool _is_dynamic = false;
		size_t _update_tick = 0;

		// Sorted by Binding Index
		MyVector<Binding> _bindings;

		VkDescriptorSetLayoutCreateFlags _flags = 0;
		VkDescriptorBindingFlags _binding_flags = 0;

		void sortBindings();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkDescriptorSetLayoutCreateFlags flags = 0;
			bool is_dynamic = false;
			MyVector<Binding> bindings = {};
			VkDescriptorBindingFlags binding_flags = 0;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		DescriptorSetLayout(CreateInfo const& ci);

		virtual ~DescriptorSetLayout() override;

		bool updateResources(UpdateContext & ctx);

		void createInstance();
		
		constexpr const MyVector<Binding>& bindings()const
		{
			return _bindings;
		}

		constexpr bool empty()const
		{
			return _bindings.empty();
		}

		constexpr VkDescriptorSetLayoutCreateFlags flags()const
		{
			return _flags;
		}

		constexpr VkDescriptorBindingFlags bindingFlags()const
		{
			return _binding_flags;
		}

		constexpr bool isDynamic() const
		{
			return _is_dynamic;
		}
	};


	template <class T>
	class SafeSharedPtrArray
	{
	protected:

		MyVector<std::shared_ptr<T>> _vector;

		template <class Q>
		friend class SafeSharedPtrArray;

	public:

		void resize(size_t s)
		{
			_vector.resize(s);
		}

		void clear()
		{
			_vector.clear();
		}

		size_t size()const
		{
			return _vector.size();
		}

		std::shared_ptr<T> getSafe(size_t i) const
		{
			if (i >= _vector.size())	return nullptr;
			return _vector[i];
		}

		std::shared_ptr<T> operator[](size_t i) const
		{
			return getSafe(i);
		}

		std::shared_ptr<T>& getRef(size_t i)
		{
			assert(i < _vector.size());
			return _vector[i];
		}

		void set(uint32_t i, std::shared_ptr<T> const& l)
		{
			(*this) += {i, l};
		}

		SafeSharedPtrArray& operator+=(std::pair<uint32_t, std::shared_ptr<T>> const& p)
		{
			if (p.first >= _vector.size())
			{
				_vector.resize(p.first + 1);
			}
			_vector[p.first] = p.second;
			return *this;
		}

		SafeSharedPtrArray operator+(std::pair<uint32_t, std::shared_ptr<T>> const& p) const
		{
			SafeSharedPtrArray res = *this;
			res += p;
			return res;
		}

		MyVector<std::shared_ptr<T>>const& asVector()const
		{
			return _vector;
		}

		auto getInstance() const
		{
			if constexpr (std::is_base_of<AbstractInstanceHolder, T>::value)
			{
				using Inst = typename T::InstanceType;
				SafeSharedPtrArray<Inst> res;
				res._vector.resize(_vector.size());
				for (size_t i = 0; i < _vector.size(); ++i)
				{
					if(_vector[i])
						res._vector[i] = _vector[i]->instance();
				}

				return res;
			}
			else
			{
				return nullptr;
			}
		}
	};

	using MultiDescriptorSetsLayoutsInstances = SafeSharedPtrArray<DescriptorSetLayoutInstance>;
	using MultiDescriptorSetsLayouts = SafeSharedPtrArray<DescriptorSetLayout>;
}
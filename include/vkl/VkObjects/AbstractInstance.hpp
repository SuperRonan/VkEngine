#pragma once

#include <vkl/App/VkApplication.hpp>
#include <mutex>

#include <that/stl_ext/pointer.hpp>
#include <that/core/Strings.hpp>

#include <vkl/Generated/VulkanTypeStructHelper.hpp>

namespace vkl
{
	class AbstractInstance : public VkObject
	{
	protected:

		size_t _creation_tick = {};
		std::vector<Callback> _destruction_callbacks = {};

		template <that::concepts::StringLike StringLike>
		constexpr AbstractInstance(VkApplication* app, StringLike&& name = "", size_t creation_tick = 0) :
			VkObject(app, std::forward<StringLike>(name)),
			_creation_tick(creation_tick)
		{}

	public:

		virtual ~AbstractInstance() override
		{}

		void callDestructionCallbacks()
		{
			for (auto& ic : _destruction_callbacks)
			{
				ic.callback();
			}
		}

		void addDestructionCallback(Callback const& ic)
		{
			_destruction_callbacks.push_back(ic);
		}

		void removeDestructionCallbacks(const void * ptr)
		{
			for (auto it = _destruction_callbacks.begin(); it < _destruction_callbacks.end(); ++it)
			{
				if (it->id == ptr)
				{
					it = _destruction_callbacks.erase(it);
				}
			}
		}

		size_t creationTick() const
		{
			return _creation_tick;
		}

		static void RegisterName(VkApplication* app, VkObjectType type, uint64_t handle, const char* name);
	};

	class AbstractVulkanInstance : public AbstractInstance
	{
	public:

		using Parent = AbstractInstance;

	protected:

		void* _handle = VK_NULL_HANDLE;

		template <that::concepts::StringLike StringLike>
		constexpr AbstractVulkanInstance(VkApplication* app, StringLike&& name = "", size_t create_tick = 0) :
			Parent(app, std::forward<StringLike>(name), create_tick)
		{

		}

	public:

		virtual VkObjectType objectType() = 0;

		void registerName(const char* name)
		{
			return RegisterName(application(), objectType(), reinterpret_cast<uint64_t>(_handle), name);
		}

		void registerName()
		{
			return registerName(name().c_str());
		}

		constexpr void* handle() const
		{
			return _handle;
		}

		constexpr void*& handle()
		{
			return _handle;
		}
	};

	template <class VkHandle>
	class InstanceBase : public AbstractVulkanInstance
	{
	public:
		using Parent = AbstractVulkanInstance;
		using HandleType = VkHandle;
		static constexpr const VkObjectType ObjectType = vku2::GetObjectType<HandleType>();
	protected:

		template <that::concepts::StringLike StringLike>
		constexpr InstanceBase(VkApplication* app, StringLike&& name = "", size_t create_tick = 0) :
			Parent(app, std::forward<StringLike>(name), create_tick)
		{

		}

	public:


		virtual VkObjectType objectType() override
		{
			return ObjectType;
		}

		HandleType handle() const
		{
			return reinterpret_cast<HandleType>(_handle);
		}

		HandleType& handle()
		{
			return reinterpret_cast<HandleType&>(_handle);
		}

		operator HandleType() const
		{
			return handle();
		}
	};

	class UpdateContext;

	struct UpdateResourcesResult
	{
		bool holds_instance = false;
		bool invalidated = false;
		bool created = false;
		bool cached = false;
	};

	class AbstractInstanceHolder : public VkObject
	{
	protected:

		std::map<uintptr_t, Callback::Function> _invalidation_callbacks = {};
		mutable std::mutex _mutex;

		Dyn<bool> _hold_instance = {};
		size_t _latest_update_tick = {};
		UpdateResourcesResult _latest_update_res;

		std::shared_ptr<AbstractInstance> _instance = {};

		virtual void updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res);

		template <that::concepts::StringLike StringLike>
		constexpr AbstractInstanceHolder(VkApplication * app, StringLike && name, Dyn<bool> const& hold_instance) :
			VkObject(app, std::forward<StringLike>(name)),
			_hold_instance(hold_instance)
		{
			hold_instance.valueOr(true);
		}

		// Create a static holder to the instance
		AbstractInstanceHolder(std::shared_ptr<AbstractInstance> const& instance);

	public:


		virtual ~AbstractInstanceHolder() override;

		void callInvalidationCallbacks();

		void setInvalidationCallback(Callback const& ic);

		void removeInvalidationCallback(const void* id);

		//void removeInvalidationCallbacks(const void* id);

		void checkHoldInstance(UpdateResourcesResult& res);

		// Uptate internale resources
		// This version:
		// - Checks the update tick
		// - calls checkHoldInstance
		// - Check the invalidation
		virtual UpdateResourcesResult updateResources(UpdateContext& ctx);

		AbstractInstance* instance() const
		{
			return _instance.get();
		}

		std::shared_ptr<AbstractInstance> const& instancePtr() const
		{
			return _instance;
		}

		constexpr const Dyn<bool>& holdInstance() const
		{
			return _hold_instance;
		}

		constexpr Dyn<bool>& holdInstance()
		{
			return _hold_instance;
		}

		virtual void destroyInstanceIFN();

		std::mutex& mutex()
		{
			return _mutex;
		}

		std::mutex const& mutex() const
		{
			return _mutex;
		}
	};

	template <std::derived_from<AbstractInstance> Instance>
		// requires std::is_pointer_interconvertible_base_of<AbstractInstance, Instance>::value
	class InstanceHolder : public AbstractInstanceHolder
	{
	protected:

		template <that::concepts::StringLike StringLike>
		constexpr InstanceHolder(VkApplication* app, StringLike&& name, Dyn<bool> const& hold_instance) :
			AbstractInstanceHolder(app, std::forward<StringLike>(name), hold_instance)
		{}

		InstanceHolder(Instance* other):
			AbstractInstanceHolder(other)
		{

		}

	public:

		using InstanceType = Instance;


		Instance* instance() const
		{
			return static_cast<Instance*>(_instance.get());
		}

		std::shared_ptr<Instance> const& instancePtr() const
		{
			return std::reinterpret_pointer_downcast<Instance>(_instance);
		}
	};

	template <std::derived_from<AbstractInstanceHolder> Descriptor>
	using DescriptorOrInstanceRawPointer = RawPointerVariant<Descriptor, typename Descriptor::InstanceType>;
	
	template <std::derived_from<AbstractInstanceHolder> Descriptor>
	using DescriptorOrInstanceUniquePointer = UniquePointerVariant<Descriptor, typename Descriptor::InstanceType>;

	template <std::derived_from<AbstractInstanceHolder> Descriptor>
	using DescriptorOrInstanceSharedPointer = SharedPointerVariant<Descriptor, typename Descriptor::InstanceType>;

	template <std::derived_from<AbstractInstanceHolder> Descriptor>
	using DescriptorOrInstanceWeakPointer = WeakPointerVariant<Descriptor, typename Descriptor::InstanceType>;

	template <template <class> class _Pointer, std::derived_from<AbstractInstanceHolder> Descriptor>
	using DescriptorOrInstanceGenPointer = GenPointerVariant<_Pointer, Descriptor, typename Descriptor::InstanceType>;

#define VKL_DEFINE_DESCRIPTOR_INSTANCE_POINTERS_2(Name, Desc) \
	using Name##OrInstanceRawPointer = DescriptorOrInstanceRawPointer<Desc>;\
	using Name##OrInstanceUniquePointer = DescriptorOrInstanceUniquePointer<Desc>;\
	using Name##OrInstanceSharedPointer = DescriptorOrInstanceSharedPointer<Desc>;\
	using Name##OrInstanceWeakPointer = DescriptorOrInstanceWeakPointer<Desc>;\
	template <template <class> class _Pointer> \
	using Name##OrInstancePointer = DescriptorOrInstanceGenPointer<_Pointer, Desc>;

#define VKL_DEFINE_DESCRIPTOR_INSTANCE_POINTERS(Desc) VKL_DEFINE_DESCRIPTOR_INSTANCE_POINTERS_2(Desc, Desc)

	template <std::derived_from<AbstractInstanceHolder> Descriptor>
	typename Descriptor::InstanceType* GetInstance(DescriptorOrInstanceRawPointer<Descriptor> ptr)
	{
		assert(ptr);
		return ptr.visit(std::overloads{
			[](Descriptor& desc){return desc.instance();},
			[](typename Descriptor::InstanceType& inst){return &inst;}
		});
	}

	template <std::derived_from<AbstractInstanceHolder> Descriptor>
	std::shared_ptr<typename Descriptor::InstanceType> const& GetInstance(DescriptorOrInstanceSharedPointer<Descriptor> const& ptr)
	{
		assert(ptr);
		if (Descriptor* d = ptr.is<Descriptor>())
		{
			return d->instancePtr();
		}
		else
		{
			ptr.reinterpretAs<typename Descriptor::InstanceType>();
		}
	}
}
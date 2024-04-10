#pragma once

#include <Core/Execution/ExecutionContext.hpp>
#include <Core/Commands/ResourceUsageList.hpp>

#include <unordered_map>
#include <vector>
#include <deque>

namespace vkl
{
	//enum class BarrierMergePolicy
	//{
	//	Never = 0,
	//	SameNature = 1, // if same read and write (and same layout)
	//	WhenContiguous = 2, // Merge all contiguous accesses (if same layout for image)
	//	AsMuchAsPossible = 3, // On barrier per buffer, multiple per image (dependent on layout)
	//	Always = 4, // One barrier per resource, might break with multiple usage of the same image subressource range
	//};

	//class AbstractSynchronizationHelper
	//{
	//protected:
	//	
	//	struct ImageSubRangeState
	//	{
	//		Range32u range = {};
	//		ResourceState2 state = {};
	//		std::optional<ResourceState2> end_state = {};
	//	};

	//public:
	//	
	//	virtual void addSynch(ResourceInstance const& r) = 0;

	//	template <class ResourceIt, class ResourceIt2 = ResourceIt>
	//	void addSynch(ResourceIt it, ResourceIt2 const& end)
	//	{
	//		while (it != end)
	//		{
	//			addSynch(*it);
	//			++it;
	//		}
	//	}

	//	virtual void record() = 0;
	//};

	//// Does not handle very well synchronizing multiple times the same resource
	//class SynchronizationHelperV1 : public AbstractSynchronizationHelper
	//{
	//protected:

	//	std::vector<ResourceInstance> _resources;
	//	std::vector<VkImageMemoryBarrier2> _images_barriers;
	//	std::vector<VkBufferMemoryBarrier2> _buffers_barriers;

	//	ExecutionContext& _ctx;

	//public:

	//	SynchronizationHelperV1(ExecutionContext& ctx) :
	//		_ctx(ctx)
	//	{}

	//	virtual void addSynch(ResourceInstance const& r) final override;

	//	virtual void record() final override;
	//};

	//class BufferSynchronizationHelper
	//{
	//protected:

	//	

	//	template <class K, class V>
	//	using Map = std::unordered_map<K, V>;

	//public:

	//	virtual void add(std::shared_ptr<BufferInstance> const& bi, Buffer::Range const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state) = 0;

	//	virtual std::vector<VkBufferMemoryBarrier2> commit() = 0;

	//	virtual ~BufferSynchronizationHelper() = default;
	//};

	//class ImageSynchronizationHelper
	//{
	//protected:

	//	struct ImageLinearRangeState
	//	{
	//		// Could be on mips or layers
	//		Range32u range = {};
	//		ResourceState2 state = {};
	//		std::optional<ResourceState2> end_state = {};
	//	};

	//	struct ImageSubRangeState
	//	{
	//		VkImageSubresourceRange range = MakeZeroImageSubRange();
	//		ResourceState2 state = {};
	//		std::optional<ResourceState2> end_state = {};
	//	};

	//	template <class K, class V>
	//	using Map = std::unordered_map<K, V>;

	//public:

	//	void addv(std::shared_ptr<ImageViewInstance> const& ivi, ResourceState2 const& state, std::optional<ResourceState2> const& end_state);

	//	virtual void add(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state) = 0;

	//	virtual std::vector<VkImageMemoryBarrier2> commit() = 0;

	//	virtual ~ImageSynchronizationHelper() = default;
	//};

	//class BasicBufferSynchronizationHelper final : public BufferSynchronizationHelper
	//{
	//protected:
	//	
	//	ExecutionContext & _ctx;

	//public:
	//};

	//class FullyMergedBufferSynchronizationHelper final : public BufferSynchronizationHelper
	//{
	//protected:

	//	ExecutionContext& _ctx;
	//	Map<std::shared_ptr<BufferInstance>, BufferSubRangeState> _buffers;

	//public:

	//	FullyMergedBufferSynchronizationHelper(ExecutionContext & ctx);

	//	virtual void add(std::shared_ptr<BufferInstance> const& bi, Buffer::Range const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state) final override;

	//	virtual std::vector<VkBufferMemoryBarrier2> commit() final override;

	//	virtual ~FullyMergedBufferSynchronizationHelper() final override = default;
	//};

	//class FullyMergedImageSynchronizationHelper final : public ImageSynchronizationHelper
	//{
	//protected:

	//	ExecutionContext & _ctx;
	//	Map<std::shared_ptr<ImageInstance>, ImageSubRangeState> _images;
	//
	//public:

	//	FullyMergedImageSynchronizationHelper(ExecutionContext& ctx);

	//	virtual void add(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state) final override;

	//	virtual std::vector<VkImageMemoryBarrier2> commit() final override;

	//	virtual ~FullyMergedImageSynchronizationHelper() final override = default;
	//};

	// Can synchronize correctly the same resource multiple times
	//class SynchronizationHelperV2 : public AbstractSynchronizationHelper
	//{
	//protected:

	//	struct ImageNewState
	//	{


	//		struct MipState
	//		{
	//			std::deque<LayersNewState> states;
	//		};

	//		// One per mip level
	//		// - Layers
	//		std::vector<MipState> states;
	//		// For now single aspect 
	//		VkImageAspectFlags aspect;
	//	};

	//	struct BufferNewState
	//	{
	//		struct RangeState
	//		{
	//			Buffer::Range range = {};
	//			ResourceState2 state = {};
	//			std::optional<ResourceState2> end_state = {};
	//		};
	//		std::deque<RangeState> states;
	//	};

	//	Map<std::shared_ptr<ImageInstance>, ImageNewState> _images;
	//	Map<std::shared_ptr<BufferInstance>, BufferNewState> _buffers;

	//	ExecutionContext& _ctx;

	//	bool checkBuffersIntegrity() const;

	//	bool checkImagesIntegrity() const;

	//public:

	//	SynchronizationHelperV2(ExecutionContext& ctx) :
	//		_ctx(ctx)
	//	{}

	//	virtual void addSynch(Resource const& r) final override;

	//	virtual void record() final override;
	//};

	//class ModularSynchronizationHelper : public AbstractSynchronizationHelper
	//{
	//protected:

	//	ExecutionContext & _ctx;
	//	BarrierMergePolicy _policy;

	//	union
	//	{
	//		FullyMergedBufferSynchronizationHelper _fully_merged_buffers;
	//	};

	//	union
	//	{
	//		FullyMergedImageSynchronizationHelper _fully_merged_images;
	//	};

	//public:

	//	ModularSynchronizationHelper(ExecutionContext & ctx, BarrierMergePolicy policy = BarrierMergePolicy::Always);

	//	~ModularSynchronizationHelper();
	//	
	//	virtual void addSynch(ResourceInstance const& r) final override;

	//	virtual void record() final override;
	//};



	////using SynchronizationHelper = SynchronizationHelperV1;
	//using SynchronizationHelper = ModularSynchronizationHelper;

	//static_assert(std::is_base_of<AbstractSynchronizationHelper, SynchronizationHelper>::value);

	bool InlineSynchronizeBuffer(ExecutionContext & ctx, BufferAndRangeInstance const& bari, ResourceState2 const& begin_state, std::optional<ResourceState2> const& end_state = {});
	bool InlineSynchronizeImage(ExecutionContext& ctx, std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& begin_state, std::optional<ResourceState2> const& end_state = {});
	bool InlineSynchronizeImageView(ExecutionContext& ctx, std::shared_ptr<ImageViewInstance> const& ivi, ResourceState2 const& begin_state, std::optional<ResourceState2> const& end_state = {});

	class SynchronizationHelper
	{
	protected:

		ExecutionContext * _ctx = nullptr;
		MyVector<VkBufferMemoryBarrier2> _buffers;
		MyVector<VkImageMemoryBarrier2> _images;
		
		BufferUsageFunction getBufferProcessFunction();

		ImageUsageFunction getImageProcessFunction();


	public:

		void reset(ExecutionContext * ctx);

		void commit(AbstractBufferUsageList const& buffers);

		void commit(AbstractImageUsageList const& images);

		void commit(ResourceUsageList const& resource_list);

		void record();
	};
} // namespace vkl

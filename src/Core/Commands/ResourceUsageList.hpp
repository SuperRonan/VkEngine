#pragma once

#include <Core/VulkanCommons.hpp>
#include <unordered_map>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/ImageView.hpp>

namespace vkl
{
	enum class ResourceUsageMergePolicy
	{
		Never = 0,
		SameNature = 1, // if same read and write (and same layout)
		WhenContiguous = 2, // Merge all contiguous accesses (if same layout for image)
		AsMuchAsPossible = 3, // On barrier per buffer, multiple per image (dependent on layout)
		Always = 4, // One barrier per resource, might break with multiple usage of the same image subressource range
	};

	
	
	
	
	
	struct BufferSubRangeState
	{
		Buffer::Range range = {};
		ResourceState2 state = {};
		std::optional<ResourceState2> end_state = {};
	};

	struct ImageLinearRangeState
	{
		// Could be on mips or layers
		Range32u range = {};
		ResourceState2 state = {};
		std::optional<ResourceState2> end_state = {};
	};

	struct ImageSubRangeState
	{
		VkImageSubresourceRange range = MakeZeroImageSubRange();
		ResourceState2 state = {};
		std::optional<ResourceState2> end_state = {};
	};

	struct ResourceUsage
	{
		ResourceState2 begin_state = {};
		std::optional<ResourceState2> end_state = {};
		VkFlags64 usage = 0;
	};

	struct BufferUsage
	{
		BufferAndRangeInstance bari;
		ResourceState2 begin_state = {};
		std::optional<ResourceState2> end_state = {};
		VkBufferUsageFlags2KHR usage = 0;
	};

	struct ImageUsage
	{
		std::shared_ptr<ImageInstance> ii;
		std::optional<VkImageSubresourceRange> range = {};
		ResourceState2 begin_state = {};
		std::optional<ResourceState2> end_state = {};
		VkImageUsageFlags usage = 0;
	};

	struct ImageViewUsage
	{
		std::shared_ptr<ImageViewInstance> ivi;
		ResourceState2 begin_state = {};
		std::optional<ResourceState2> end_state = {};
		VkImageUsageFlags usage = 0;
	};
	
	// A pointer will do for now, but a more general iteraator would be good
	using BufferUsageFunction = std::function<void(std::shared_ptr<BufferInstance>, const BufferSubRangeState*, size_t)>;
	using ImageUsageFunction = std::function<void(std::shared_ptr<ImageInstance>, const ImageSubRangeState*, size_t)>;
	
	
	class AbstractBufferUsageList
	{
	protected:


	public:

		virtual void add(BufferAndRangeInstance const& bari, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkBufferUsageFlags2KHR usages = 0) = 0;

		virtual void iterate(BufferUsageFunction const& fn) const = 0;

		virtual void clear() = 0;

	};

	class FullyMergedBufferUsageList final : public AbstractBufferUsageList
	{
	protected:

		std::unordered_map<std::shared_ptr<BufferInstance>, BufferSubRangeState> _buffers;

	public:

		virtual void add(BufferAndRangeInstance const& bari, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkBufferUsageFlags2KHR usages = 0) final override;

		virtual void iterate(BufferUsageFunction const& fn) const final override;

		virtual void clear() final override;
	};











	class AbstractImageUsageList
	{
	protected:

		

	public:

		virtual void add(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkImageUsageFlags usages = 0) = 0;
		
		void addv(std::shared_ptr<ImageViewInstance> const& ivi, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkImageUsageFlags usages = 0)
		{
			add(ivi->image(), ivi->createInfo().subresourceRange, state, end_state, usages);
		}

		virtual void iterate(ImageUsageFunction const& fn) const = 0;

		virtual void clear() = 0;

	};

	class FullyMergedImageUsageList final : public AbstractImageUsageList
	{
	protected:

		std::unordered_map<std::shared_ptr<ImageInstance>, ImageSubRangeState> _images;

	public:

		virtual void add(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkImageUsageFlags usages = 0) final override;
		
		virtual void iterate(ImageUsageFunction const& fn) const final override;

		virtual void clear() final override;
	};








	class AbstractResourceUsageList
	{
	protected:



	public:

		


		virtual void addBuffer(BufferAndRangeInstance const& bari, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkBufferUsageFlags2KHR usages = 0) = 0;

		virtual void addImage(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkImageUsageFlags usages = 0) = 0;
		
		void addView(std::shared_ptr<ImageViewInstance> const& ivi, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkImageUsageFlags usages = 0)
		{
			addImage(ivi->image(), ivi->createInfo().subresourceRange, state, end_state, usages);
		}

		void add(BufferUsage const& bu) 
		{
			addBuffer(bu.bari, bu.begin_state, bu.end_state, bu.usage);
		}

		void add(ImageUsage const& iu)
		{
			addImage(iu.ii, iu.range.value_or(iu.ii->defaultSubresourceRange()), iu.begin_state, iu.end_state, iu.usage);
		}

		void add(ImageViewUsage const& ivu)
		{
			addView(ivu.ivi, ivu.begin_state, ivu.end_state, ivu.usage);
		}

		AbstractResourceUsageList& operator+=(BufferUsage const& bu)
		{
			add(bu);
			return *this;
		}

		AbstractResourceUsageList& operator+=(ImageUsage const& iu)
		{
			add(iu);
			return *this;
		}

		AbstractResourceUsageList& operator+=(ImageViewUsage const& ivu)
		{
			add(ivu);
			return *this;
		}
		
		virtual void iterateOnBuffers(BufferUsageFunction const& fn) const = 0;

		virtual void iterateOnImages(ImageUsageFunction const& fn) const = 0;

		virtual void clear() = 0;

	};


	class ModularResourceUsageList : public AbstractResourceUsageList
	{
	protected:

		ResourceUsageMergePolicy _policy = ResourceUsageMergePolicy::Always;

		FullyMergedBufferUsageList _buffers;
		FullyMergedImageUsageList _images;

	public:

		ModularResourceUsageList();

		

		virtual void addBuffer(BufferAndRangeInstance const& bari, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkBufferUsageFlags2KHR usages = 0) final override;

		virtual void addImage(std::shared_ptr<ImageInstance> const& ii, VkImageSubresourceRange const& range, ResourceState2 const& state, std::optional<ResourceState2> const& end_state = {}, VkImageUsageFlags usages = 0) final override;

		virtual void iterateOnBuffers(BufferUsageFunction const& fn) const final override;

		virtual void iterateOnImages(ImageUsageFunction const& fn) const final override;

		virtual void clear() final override;
	};

	using ResourceUsageList = ModularResourceUsageList;
}
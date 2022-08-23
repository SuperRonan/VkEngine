
#include "Command.hpp"
#include <cassert>

namespace vkl
{
	ResourceState& ExecutionContext::getBufferState(VkBuffer b)
	{
		if (!_buffer_states.contains(b))
		{
			ResourceState not_yet_used{};
			_buffer_states[b] = not_yet_used;
		}
		return _buffer_states[b];
	}

	ResourceState& ExecutionContext::getImageState(VkImageView i)
	{
		if (!_image_states.contains(i))
		{
			ResourceState not_yet_used{};
			_image_states[i] = not_yet_used;
		}
		return _image_states[i];
	}

	void ShaderCommand::writeDescriptorSets()
	{
		std::vector<std::shared_ptr<DescriptorSetLayout>> set_layouts = _pipeline->program()->setLayouts();
		_desc_pools.resize(set_layouts.size());
		_desc_sets.resize(set_layouts.size());
		for (size_t i = 0; i < set_layouts.size(); ++i)
		{
			_desc_pools[i] = std::make_shared<DescriptorPool>(set_layouts[i]);
			_desc_sets[i] = std::make_shared<DescriptorSet>(set_layouts[i], _desc_pools[i]);
		}
		const size_t N = _bindings.size();

		// TODO Resolve named bindings
		//{
		//	_pipeline->program();
		//	for (size_t i = 0; i < N; ++i)
		//	{
		//		ResourceBinding& binding = _bindings[i];
		//		if (!binding.resolved())
		//		{
		//			assert(!binding.name().empty());
		//		}
		//	}
		//}

		std::vector<VkDescriptorBufferInfo> buffers;
		buffers.reserve(N);
		std::vector<VkDescriptorImageInfo> images;
		images.reserve(N);

		std::vector<VkWriteDescriptorSet> writes;
		writes.reserve(N);

		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding& b = _bindings[i];
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = *_desc_sets[b.set()],
				.dstBinding = b.binding(),
				.dstArrayElement = 0, // TODO
				.descriptorCount = 1,
				.descriptorType = b.vkType(),
			};
			if (b.isBuffer())
			{
				assert(b.buffers().size() == 1);
				buffers.emplace_back(VkDescriptorBufferInfo{
					.buffer = *b.buffers().front(),
					.offset = 0,
					.range = VK_WHOLE_SIZE,
					});
				VkDescriptorBufferInfo& info = buffers.back();
				write.pBufferInfo = &info;
			}
			else if (b.isImage())
			{
				VkDescriptorImageInfo info;
				if (!b.samplers().empty())
				{
					info.sampler = *b.samplers().front();
				}
				if (!b.images().empty())
				{
					info.imageView = *b.images().front();
					info.imageLayout = b.state()._layout;
					images.push_back(info);
					write.pImageInfo = &images.back();
				}
			}
			else
			{
				assert(false);
			}

			writes.push_back(write);
		}

		vkUpdateDescriptorSets(_app->device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
	}

	void ShaderCommand::declareDescriptorSetsResources()
	{
		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			const ResourceBinding& b = _bindings[i];
			_resources.push_back(b.resource());
		}
	}

	void ShaderCommand::extractBindingsFromReflection()
	{
		const auto& program = *_pipeline->program();
		const auto& shaders = program.shaders();
		for (size_t s = 0; s < shaders.size(); ++s)
		{
			
		}
	}

	void DeviceCommand::recordInputSynchronization(CommandBuffer & cmd, ExecutionContext & context)
	{
		std::vector<VkImageMemoryBarrier> image_barriers;
		std::vector<VkBufferMemoryBarrier> buffer_barriers;
		for (size_t i = 0; i < _resources.size(); ++i)
		{
			auto& r = _resources[i];
			const ResourceState next = r._state;
			const ResourceState prev = [&]() {
				if (r.isImage())
				{
					return context.getImageState(*r._images[0].get());
				}
				else if(r.isBuffer())
				{
					return context.getBufferState(*r._buffers[0].get());
				}
				else
				{
					assert(false);
					// ???
					return ResourceState{};
				}
			}();
			if (stateTransitionRequiresSynchronization(prev, next, r.isImage()))
			{
				if (r.isImage())
				{
					VkImageMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.pNext = nullptr,
						.srcAccessMask = prev._access,
						.dstAccessMask = next._access,
						.oldLayout = prev._layout,
						.newLayout = next._layout,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.image = *r._images[0]->image(),
						.subresourceRange = r._images[0]->range(),
					};
					image_barriers.push_back(barrier);
				}
				else if(r.isBuffer())
				{
					VkBufferMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.pNext = nullptr,
						.srcAccessMask = prev._access,
						.dstAccessMask = next._access,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = *r._buffers[0],
						.offset = 0,
						.size = VK_WHOLE_SIZE,
					};
					buffer_barriers.push_back(barrier);
				}
			}
		}
		if (!image_barriers.empty() || !buffer_barriers.empty())
		{
			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
				0, nullptr,
				(uint32_t)buffer_barriers.size(), buffer_barriers.data(),
				(uint32_t)image_barriers.size(), image_barriers.data());
		}
	}

	void ShaderCommand::recordBindings(CommandBuffer & cmd, ExecutionContext& context)
	{
		vkCmdBindPipeline(cmd, _pipeline->binding(), *_pipeline);

		std::vector<VkDescriptorSet> desc_sets(_desc_sets.size());
		for (size_t i = 0; i < desc_sets.size(); ++i)	desc_sets[i] = *_desc_sets[i];
		vkCmdBindDescriptorSets(cmd, _pipeline->binding(), _pipeline->program()->pipelineLayout(), 0, (uint32_t)_desc_sets.size(), desc_sets.data(), 0, nullptr);

		VkPipelineStageFlags pc_stages = 0;
		for (const auto& pc_range : _pipeline->program()->pushConstantRanges())
		{
			pc_stages |= pc_range.stageFlags;
		}
		vkCmdPushConstants(cmd, _pipeline->program()->pipelineLayout(), pc_stages, 0, (uint32_t)_push_constants_data.size(), _push_constants_data.data());
	}

	void ComputeCommand::recordCommandBuffer(CommandBuffer & cmd, ExecutionContext& context)
	{
		recordInputSynchronization(cmd, context);
		recordBindings(cmd, context);

		const VkExtent3D workgroups = _dispatch_threads ? VkExtent3D{
			.width = std::moduloCeil(_dispatch_size.width, _program->localSize().width),
			.height = std::moduloCeil(_dispatch_size.height, _program->localSize().height),
			.depth = std::moduloCeil(_dispatch_size.depth, _program->localSize().depth),
		} : _dispatch_size;
		vkCmdDispatch(cmd, workgroups.width, workgroups.height, workgroups.depth);
	}

	void ComputeCommand::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCurrentCommandBuffer();
		recordCommandBuffer(*cmd, context);
	}

	void GraphicsCommand::recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context)
	{
		recordInputSynchronization(cmd, context);
		recordBindings(cmd, context);

		VkExtent2D render_area = {
			.width = _framebuffer->extent().width,
			.height = _framebuffer->extent().height,
		};

		std::vector<VkClearValue> clear_values(_framebuffer->size());
		for (size_t i = 0; i < clear_values.size(); ++i)
		{
			clear_values[i] = VkClearValue{
				.color = VkClearColorValue{.int32{0, 0, 0, 0}},
			};
		}

		VkRenderPassBeginInfo begin = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = *_render_pass,
			.framebuffer = *_framebuffer,
			.renderArea = VkRect2D{.offset = VkOffset2D{0, 0}, .extent = render_area},
			.clearValueCount = (uint32_t)clear_values.size(),
			.pClearValues = clear_values.data(),
		};

		vkCmdBeginRenderPass(cmd, &begin, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *_pipeline);
			recordBindings(cmd, context);
			recordDraw(cmd, context);
		}
		vkCmdEndRenderPass(cmd);
	}

	void GraphicsCommand::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCurrentCommandBuffer();
		recordCommandBuffer(*cmd, context);
	}

	void DrawMeshCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	{
		for (auto& mesh : _meshes)
		{
			mesh->recordBind(cmd);
			vkCmdDrawIndexed(cmd, (uint32_t)mesh->indicesSize(), 1, 0, 0, 0);
		}
	}

	void FragCommand::recordDraw(CommandBuffer& cmd, ExecutionContext& context)
	{
		vkCmdDrawIndexed(cmd, 4, 1, 0, 0, 0);
	}

	void BlitImage::init()
	{
		_resources.push_back(Resource{
			._images = {_src},
			._state = ResourceState{
				._access = VK_ACCESS_TRANSFER_READ_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT
			},
		});
		_resources.push_back(Resource{
			._images = {_dst},
			._state = ResourceState{
				._access = VK_ACCESS_TRANSFER_WRITE_BIT,
				._layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				._stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			},
		});
	}

	void BlitImage::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCurrentCommandBuffer();
		recordInputSynchronization(*cmd, context);

		vkCmdBlitImage(*cmd, 
			*_src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			*_dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			(uint32_t)_regions.size(), _regions.data(), _filter
		);
	}
}
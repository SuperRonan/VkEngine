#pragma once

struct VkDrawIndirectCommand {
	uint32_t	vertexCount;
	uint32_t	instanceCount;
	uint32_t	firstVertex;
	uint32_t	firstInstance;
};

struct VkDrawIndexedIndirectCommand {
	uint32_t	indexCount;
	uint32_t	instanceCount;
	uint32_t	firstIndex;
	int32_t		vertexOffset;
	uint32_t	firstInstance;
};
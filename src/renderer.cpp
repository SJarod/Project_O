#include "renderer.hpp"

void Renderer::destroyFloatingBufferObject(VertexBuffer& vbo)
{
	const VkDevice& device = ldevice.getVkLDevice();
	vkDestroyBuffer(device, vbo.buffer, nullptr);
}

void Renderer::destroyBufferObject(int index)
{
	destroyFloatingBufferObject(vbos[index]);
	vbos.erase(index);
}

void Renderer::createVertexBufferObject(uint32_t vertexNum, Vertex* vertices)
{
	// this buffer is a CPU accessible buffer (temporary buffer to later load the data to the GPU)
	VertexBuffer stagingVBO = createFloatingBufferObject(vertexNum,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,	// used for memory transfer operation
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// populating the CPU accessible buffer
	populateBufferObject(stagingVBO, vertices);

	// creating a device (GPU) local buffer (on the specific device of the renderer)
	// store this vertex buffer to keep a reference
	VertexBuffer& vbo = createBufferObject(vertexNum,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,	// memory transfer operation
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// copying the staging buffer data into the device local buffer

	// using a command buffer to transfer the data
	CommandBuffer cbo = commandPool.createFloatingCommandBuffer();

	// copy the staging buffer (CPU accessible) into the GPU buffer (GPU memory)
	cbo.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkBufferCopy copyRegion = {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = stagingVBO.bufferSize
		};
		vkCmdCopyBuffer(cbo.getVkBuffer(), stagingVBO.buffer, vbo.buffer, 1, &copyRegion);

	cbo.endRecord();

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cbo.getVkBuffer()
	};
	ldevice.submitCommandToGraphicsQueue(submitInfo);
	ldevice.waitGraphicsQueue();

	commandPool.destroyFloatingCommandBuffer(cbo);
	destroyFloatingBufferObject(stagingVBO);
}

Renderer::Renderer()
	: ldevice(low), commandPool(ldevice), pipeline(ldevice), allocator(ldevice)
{
}

void Renderer::create()
{
	// create the rendering instance first using low.create()
	ldevice.create();
	commandPool.create();
	pipeline.create();

	// default command buffer
	commandPool.createCommandBuffer();
}

void Renderer::destroy()
{
	pipeline.destroy();
	commandPool.destroy();

	for (int i = 0; i < vbos.size(); ++i)
	{
		destroyFloatingBufferObject(vbos[i]);
	}
	vbos.clear();

	allocator.destroy();

	ldevice.destroy();
	low.destroy();
}

VertexBuffer Renderer::createFloatingBufferObject(uint32_t vertexNum, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties)
{
	// out buffer object
	VertexBuffer outVbo;
	outVbo.bufferSize = sizeof(Vertex) * (size_t)vertexNum;
	outVbo.vertexNum = vertexNum;

	const VkDevice& device = ldevice.getVkLDevice();

	VkBufferCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.flags = 0,
		.size = (VkDeviceSize)outVbo.bufferSize,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	if (vkCreateBuffer(device, &createInfo, nullptr, &outVbo.buffer) != VK_SUCCESS)
		throw std::exception("Failed to create vertex buffer");

	// binding memory block
	MemoryBlock& block = allocator.getAvailableBlock(outVbo.bufferSize, outVbo.buffer, memProperties);
	outVbo.memoryOffset = block.usedSpace;
	vkBindBufferMemory(device, outVbo.buffer, block.memory, block.usedSpace);
	// marking space as taken
	block.usedSpace += outVbo.bufferSize;
	outVbo.memoryBlock = &block;

	return outVbo;
}

VertexBuffer& Renderer::createBufferObject(uint32_t vertexNum, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties)
{	
	int index = vbos.size();
	vbos[index] = createFloatingBufferObject(vertexNum, usage, memProperties);
	return vbos[index];
}

void Renderer::populateBufferObject(VertexBuffer& vbo, Vertex* vertices)
{
	const VkDevice& device = ldevice.getVkLDevice();

	// populating the VBO (using a CPU accessible memory)
	vkMapMemory(device,
		vbo.memoryBlock->memory,
		(VkDeviceSize)vbo.memoryOffset,
		(VkDeviceSize)vbo.bufferSize,
		0,
		&vbo.vertices);

	// TODO : flush memory
	memcpy(vbo.vertices, vertices, vbo.bufferSize);
	// TODO : invalidate memory before reading in the pipeline

	vkUnmapMemory(device, vbo.memoryBlock->memory);
}

void Renderer::render()
{
	pipeline.drawFrame(commandPool.getCmdBufferByIndex(0), vbos);
}
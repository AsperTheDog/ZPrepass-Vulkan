#pragma once
#include <vulkan/vulkan_core.h>

#include "vulkan_memory.hpp"

class VulkanDevice;

class VulkanBuffer
{
public:
	[[nodiscard]] VkMemoryRequirements getMemoryRequirements() const;
	
	void allocateFromIndex(uint32_t memoryIndex);
	void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences memoryProperties);

	void* map(VkDeviceSize size, VkDeviceSize offset);
	void unmap();

	[[nodiscard]] bool isMemoryMapped() const;
	[[nodiscard]] void* getMappedData() const;
	[[nodiscard]] uint32_t getID() const;
	[[nodiscard]] VkDeviceSize getSize() const;

private:
	VulkanBuffer() = default;
	VulkanBuffer(VulkanDevice& device, VkBuffer vkHandle, VkDeviceSize size);
	void setBoundMemory(const MemoryChunk::MemoryBlock& memoryRegion);
	void free();

	uint32_t m_id = 0;

	MemoryChunk::MemoryBlock m_memoryRegion;
	VulkanDevice* m_device = nullptr;
	VkDeviceSize m_size = 0;

	VkBuffer m_vkHandle = VK_NULL_HANDLE;

	void* m_mappedData = nullptr;

	inline static uint32_t s_idCounter = 0;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};


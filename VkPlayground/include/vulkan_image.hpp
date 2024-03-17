#pragma once
#include <vulkan/vulkan_core.h>

#include "vulkan_memory.hpp"

class VulkanDevice;

class VulkanImage
{
public:
	[[nodiscard]] VkMemoryRequirements getMemoryRequirements() const;
	
	void allocateFromIndex(uint32_t memoryIndex);
	void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences memoryProperties);

	VkImageView createImageView(VkFormat format, VkImageAspectFlags aspectFlags);
	void freeImageView(VkImageView imageView);

	void transitionLayout(const VkImageLayout layout, const VkImageAspectFlags aspectFlags, const uint32_t srcQueueFamily, const uint32_t
	                      dstQueueFamily, uint32_t threadID);

	[[nodiscard]] uint32_t getID() const;
	[[nodiscard]] VkExtent3D getSize() const;

private:
	VulkanImage() = default;
	VulkanImage(VulkanDevice& device, VkImage vkHandle, VkExtent3D size, VkImageType type, VkImageLayout layout);
	void setBoundMemory(const MemoryChunk::MemoryBlock& memoryRegion);
	void free();

	uint32_t m_id = 0;

	MemoryChunk::MemoryBlock m_memoryRegion;
	VkExtent3D m_size{};
	VkImageType m_type;
	VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	
	VkImage m_vkHandle = VK_NULL_HANDLE;
	VulkanDevice* m_device = nullptr;

	std::vector<VkImageView> m_imageViews;

	inline static uint32_t s_idCounter = 0;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};


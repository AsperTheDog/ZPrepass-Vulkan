#pragma once
#include <vulkan/vulkan_core.h>

class VulkanDevice;

class VulkanFence
{
public:

	void reset();
	void wait();

	void free();

	[[nodiscard]] uint32_t getID() const;
	[[nodiscard]] bool isSignaled() const;

private:
	VulkanFence() = default;
	VulkanFence(VulkanDevice& device, VkFence fence, bool isSignaled);

	uint32_t m_id = 0;

	bool m_isSignaled = false;

	VkFence m_vkHandle = VK_NULL_HANDLE;
	VulkanDevice* m_device = nullptr;

	inline static uint32_t s_idCounter = 0;

	friend class VulkanDevice;
	friend class SDLWindow;
	friend class VulkanCommandBuffer;
};


#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>

#include "vulkan_device.hpp"
#include "vulkan_queues.hpp"

class VulkanGPU;

class VulkanContext
{
public:
	VulkanContext() = default;
	VulkanContext(uint32_t vulkanApiVersion, bool enableValidationLayers, const std::vector<const char*>& extensions);

	[[nodiscard]] std::vector<VulkanGPU> getGPUs() const;
	[[nodiscard]] VulkanDevice& getDevice();
	[[nodiscard]] const VulkanDevice& getDevice() const;

	VulkanDevice& createDevice(VulkanGPU gpu, const QueueFamilySelector& queues, const std::vector<const char*>& extensions, const VkPhysicalDeviceFeatures& features);

	void free();

private:
	VkInstance m_vkHandle = VK_NULL_HANDLE;
	bool m_validationLayersEnabled = false;

	VulkanDevice m_device{};

	friend class SDLWindow;
};


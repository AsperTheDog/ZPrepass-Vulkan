#include "vulkan_context.hpp"

#include <stdexcept>
#include <vector>

#include "vulkan_device.hpp"
#include "vulkan_gpu.hpp"
#include "vulkan_queues.hpp"

std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

VulkanContext::VulkanContext(const uint32_t vulkanApiVersion, const bool enableValidationLayers, const std::vector<const char*>& extensions)
	: m_validationLayersEnabled(enableValidationLayers)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = vulkanApiVersion;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (enableValidationLayers)
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
	}
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkHandle) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}
}

std::vector<VulkanGPU> VulkanContext::getGPUs() const
{
	uint32_t gpuCount = 0;
	vkEnumeratePhysicalDevices(m_vkHandle, &gpuCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	vkEnumeratePhysicalDevices(m_vkHandle, &gpuCount, physicalDevices.data());

	std::vector<VulkanGPU> gpus;
	gpus.reserve(physicalDevices.size());
	for (const auto& physicalDevice : physicalDevices)
	{
		gpus.push_back(VulkanGPU(physicalDevice));
	}
	return gpus;
}

VulkanDevice& VulkanContext::getDevice()
{
	return m_device;
}

const VulkanDevice& VulkanContext::getDevice() const
{
	return m_device;
}

VulkanDevice& VulkanContext::createDevice(const VulkanGPU gpu, const QueueFamilySelector& queues, const std::vector<const char*>& extensions, const VkPhysicalDeviceFeatures& features)
{
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	if (m_validationLayersEnabled)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}
	if (!extensions.empty())
	{
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
	}
	else
	{
		deviceCreateInfo.enabledExtensionCount = 0;
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (const uint32_t index : queues.getUniqueIndices())
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfo.queueCount = static_cast<uint32_t>(queues.m_selections[index].priorities.size());
		queueCreateInfo.pQueuePriorities = queues.m_selections[index].priorities.data();
		if (queues.m_selections[index].familyFlags & QueueFamilyTypeBits::PROTECTED)
		{
			queueCreateInfo.flags = VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;
		}
		queueCreateInfos.push_back(queueCreateInfo);
	}
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	deviceCreateInfo.pEnabledFeatures = &features;

	VkDevice device;
	if (vkCreateDevice(gpu.m_vkHandle, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device");
	}

	m_device = VulkanDevice(gpu, device);
	return m_device;
}

void VulkanContext::free()
{
	if (m_device.m_vkHandle != VK_NULL_HANDLE)
	{
		m_device.free();
	}

	vkDestroyInstance(m_vkHandle, nullptr);
	m_vkHandle = VK_NULL_HANDLE;
}

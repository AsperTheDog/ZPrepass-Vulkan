#include "vulkan_fence.hpp"

#include "vulkan_device.hpp"

void VulkanFence::reset()
{
	vkResetFences(m_device->m_vkHandle, 1, &m_vkHandle);
	m_isSignaled = false;
}

void VulkanFence::wait()
{
	vkWaitForFences(m_device->m_vkHandle, 1, &m_vkHandle, VK_TRUE, UINT64_MAX);
	m_isSignaled = true;
}

void VulkanFence::free()
{
	vkDestroyFence(m_device->m_vkHandle, m_vkHandle, nullptr);
	m_vkHandle = VK_NULL_HANDLE;
}

uint32_t VulkanFence::getID() const
{
	return m_id;
}

bool VulkanFence::isSignaled() const
{
	return m_isSignaled;
}

VulkanFence::VulkanFence(VulkanDevice& device, const VkFence fence, const bool isSignaled)
	: m_id(s_idCounter++), m_isSignaled(isSignaled), m_vkHandle(fence), m_device(&device)
{
}

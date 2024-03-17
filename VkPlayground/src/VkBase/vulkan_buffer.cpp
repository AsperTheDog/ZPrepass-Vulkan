#include "vulkan_buffer.hpp"

#include <iostream>
#include <stdexcept>

#include "logger.hpp"
#include "vulkan_device.hpp"

VkMemoryRequirements VulkanBuffer::getMemoryRequirements() const
{
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_device->m_vkHandle, m_vkHandle, &memoryRequirements);
	return memoryRequirements;
}

void VulkanBuffer::allocateFromIndex(const uint32_t memoryIndex)
{
	Logger::pushContext("Buffer memory");
	const VkMemoryRequirements requirements = getMemoryRequirements();
	setBoundMemory(m_device->m_memoryAllocator.allocate(requirements.size, requirements.alignment, memoryIndex));
	Logger::popContext();
}

void VulkanBuffer::allocateFromFlags(const VulkanMemoryAllocator::MemoryPropertyPreferences memoryProperties)
{
	Logger::pushContext("Buffer memory");
	const VkMemoryRequirements requirements = getMemoryRequirements();
	setBoundMemory(m_device->m_memoryAllocator.searchAndAllocate(requirements.size, requirements.alignment, memoryProperties, requirements.memoryTypeBits));
	Logger::popContext();
}

bool VulkanBuffer::isMemoryMapped() const
{
	return m_mappedData != nullptr;
}

void* VulkanBuffer::getMappedData() const
{
	return m_mappedData;
}

uint32_t VulkanBuffer::getID() const
{
	return m_id;
}

VkDeviceSize VulkanBuffer::getSize() const
{
	return m_size;
}

void* VulkanBuffer::map(const VkDeviceSize size, const VkDeviceSize offset)
{
	void* data;
	vkMapMemory(m_device->m_vkHandle, m_device->getMemoryHandle(m_memoryRegion.chunk), m_memoryRegion.offset + offset, size, 0, &data);
	m_mappedData = data;
	return data;
}

void VulkanBuffer::unmap()
{
	vkUnmapMemory(m_device->m_vkHandle, m_device->getMemoryHandle(m_memoryRegion.chunk));
	m_mappedData = nullptr;
}

VulkanBuffer::VulkanBuffer(VulkanDevice& device, const VkBuffer vkHandle, const VkDeviceSize size)
	: m_id(s_idCounter++), m_device(&device), m_size(size), m_vkHandle(vkHandle)
{
	Logger::print("Created buffer " + std::to_string(m_id) + " with size " + std::to_string(m_size));
}

void VulkanBuffer::setBoundMemory(const MemoryChunk::MemoryBlock& memoryRegion)
{
	if (m_memoryRegion.size > 0)
	{
		throw std::runtime_error("Buffer already has memory bound to it!");
	}
	m_memoryRegion = memoryRegion;

	Logger::print("Bound memory to buffer " + std::to_string(m_id) + " with size " + std::to_string(m_memoryRegion.size) + " and offset " + std::to_string(m_memoryRegion.offset));
	vkBindBufferMemory(m_device->m_vkHandle, m_vkHandle, m_device->getMemoryHandle(m_memoryRegion.chunk), m_memoryRegion.offset);
}

void VulkanBuffer::free()
{
	Logger::print("Freeing buffer " + std::to_string(m_id));
	vkDestroyBuffer(m_device->m_vkHandle, m_vkHandle, nullptr);
	m_vkHandle = VK_NULL_HANDLE;

	if (m_memoryRegion.size > 0)
	{
		m_device->m_memoryAllocator.deallocate(m_memoryRegion);
		m_memoryRegion = {};
	}
}

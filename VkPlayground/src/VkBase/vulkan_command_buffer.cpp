#include "vulkan_command_buffer.hpp"

#include <array>
#include <stdexcept>
#include <vector>

#include "vulkan_buffer.hpp"
#include "vulkan_fence.hpp"
#include "vulkan_queues.hpp"
#include "vulkan_render_pass.hpp"

void VulkanCommandBuffer::beginRecording(const VkCommandBufferUsageFlags flags)
{
	if (m_isRecording)
	{
		throw std::runtime_error("Command buffer is already recording");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags;

	vkBeginCommandBuffer(m_vkHandle, &beginInfo);

	m_isRecording = true;
}

void VulkanCommandBuffer::endRecording()
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkEndCommandBuffer(m_vkHandle);

	m_isRecording = false;
}

void VulkanCommandBuffer::cmdCopyBuffer(const VulkanBuffer& source, const VulkanBuffer& destination, const std::vector<VkBufferCopy>& copyRegions)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdCopyBuffer(m_vkHandle, source.m_vkHandle, destination.m_vkHandle, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
}

void VulkanCommandBuffer::cmdPushConstant(const VkPipelineLayout layout, const VkShaderStageFlags stageFlags, const uint32_t offset, const uint32_t size, const void* pValues)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdPushConstants(m_vkHandle, layout, stageFlags, offset, size, pValues);
}

void VulkanCommandBuffer::submit(const VulkanQueue& queue, const std::vector<std::pair<VkSemaphore, VkSemaphoreWaitFlags>>& waitSemaphoreData, const std::vector<VkSemaphore>& signalSemaphores, const VulkanFence* fence) const
{
	if (m_isRecording)
	{
		throw std::runtime_error("Command buffer is still recording");
	}

	std::vector<VkSemaphore> waitSemaphores{};
	std::vector<VkPipelineStageFlags> waitStages{};
	waitSemaphores.resize(waitSemaphoreData.size());
	waitStages.resize(waitSemaphoreData.size());
	for (size_t i = 0; i < waitSemaphores.size(); i++)
	{
		waitSemaphores[i] = waitSemaphoreData[i].first;
		waitStages[i] = waitSemaphoreData[i].second;
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vkHandle;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	vkQueueSubmit(queue.m_vkHandle, 1, &submitInfo, fence != nullptr ? fence->m_vkHandle : VK_NULL_HANDLE);
}

void VulkanCommandBuffer::reset() const
{
	if (m_isRecording)
	{
		throw std::runtime_error("Command buffer is still recording");
	}

	vkResetCommandBuffer(m_vkHandle, 0);
}

void VulkanCommandBuffer::cmdBeginRenderPass(const VulkanRenderPass& renderPass, const VkFramebuffer frameBuffer, const VkExtent2D extent, const std::vector<VkClearValue>& clearValues) const
{
	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = renderPass.m_vkHandle;
	beginInfo.framebuffer = frameBuffer;
	beginInfo.renderArea.offset = { 0, 0 };
	beginInfo.renderArea.extent = extent;

    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	beginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_vkHandle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::cmdEndRenderPass() const
{
	vkCmdEndRenderPass(m_vkHandle);
}

void VulkanCommandBuffer::cmdBindPipeline(const VkPipelineBindPoint bindPoint, const VkPipeline pipeline)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdBindPipeline(m_vkHandle, bindPoint, pipeline);
}

void VulkanCommandBuffer::cmdNextSubpass()
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdNextSubpass(m_vkHandle, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::cmdPipelineBarrier(const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask, const VkDependencyFlags dependencyFlags, 
	const std::vector<VkMemoryBarrier>& memoryBarriers,
	const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers,
	const std::vector<VkImageMemoryBarrier>& imageMemoryBarriers)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdPipelineBarrier(m_vkHandle, srcStageMask, dstStageMask, dependencyFlags, static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(), static_cast<uint32_t>(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(), static_cast<uint32_t>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
}

void VulkanCommandBuffer::cmdBindVertexBuffer(const VulkanBuffer& buffer, const VkDeviceSize offset)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdBindVertexBuffers(m_vkHandle, 0, 1, &buffer.m_vkHandle, &offset);
}

void VulkanCommandBuffer::cmdBindVertexBuffers(const std::vector<VulkanBuffer>& buffers, const std::vector<VkDeviceSize>& offsets)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	std::vector<VkBuffer> vkBuffers;
	vkBuffers.reserve(buffers.size());
	for (const auto& buffer : buffers)
	{
		vkBuffers.push_back(buffer.m_vkHandle);
	}
	vkCmdBindVertexBuffers(m_vkHandle, 0, static_cast<uint32_t>(vkBuffers.size()), vkBuffers.data(), offsets.data());
}

void VulkanCommandBuffer::cmdBindIndexBuffer(const VulkanBuffer& buffer, const VkDeviceSize offset, const VkIndexType indexType)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdBindIndexBuffer(m_vkHandle, buffer.m_vkHandle, offset, indexType);
}

void VulkanCommandBuffer::cmdSetViewport(const VkViewport& viewport)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdSetViewport(m_vkHandle, 0, 1, &viewport);
}

void VulkanCommandBuffer::cmdSetScissor(const VkRect2D scissor)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdSetScissor(m_vkHandle, 0, 1, &scissor);
}

void VulkanCommandBuffer::cmdDraw(const uint32_t vertexCount, const uint32_t firstVertex)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}

	vkCmdDraw(m_vkHandle, vertexCount, 1, firstVertex, 0);
}

void VulkanCommandBuffer::cmdDrawIndexed(const uint32_t indexCount, const uint32_t  firstIndex, const int32_t  vertexOffset)
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer is not recording");
	}
	vkCmdDrawIndexed(m_vkHandle, indexCount, 1, firstIndex, vertexOffset, 0);
}

uint32_t VulkanCommandBuffer::getID() const
{
	return m_id;
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device, const VkCommandBuffer commandBuffer, const bool isSecondary, const uint32_t familyIndex, const uint32_t threadID)
	: m_vkHandle(commandBuffer), m_id(s_idCounter++), m_isSecondary(isSecondary), m_familyIndex(familyIndex), m_threadID(threadID), m_device(&device)
{
}

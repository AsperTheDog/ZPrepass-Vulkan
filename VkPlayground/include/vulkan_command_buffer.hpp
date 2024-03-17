#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>


class VulkanBuffer;
class VulkanQueue;
class VulkanFence;
class VulkanRenderPass;
class VulkanDevice;

class VulkanCommandBuffer
{
public:
	void beginRecording(VkCommandBufferUsageFlags flags);
	void endRecording();
	void submit(const VulkanQueue& queue, const std::vector<std::pair<VkSemaphore, VkSemaphoreWaitFlags>>& waitSemaphoreData, const std::vector<VkSemaphore>& signalSemaphores, const VulkanFence* fence) const;
	void reset() const;

	void cmdBeginRenderPass(const VulkanRenderPass& renderPass, VkFramebuffer frameBuffer, VkExtent2D extent, const std::vector<VkClearValue>& clearValues) const;
	void cmdEndRenderPass() const;
	void cmdBindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
	void cmdNextSubpass();
	void cmdPipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, 
		const std::vector<VkMemoryBarrier>& memoryBarriers, 
		const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers, 
		const std::vector<VkImageMemoryBarrier>& imageMemoryBarriers);

	void cmdBindVertexBuffer(const VulkanBuffer& buffer, VkDeviceSize offset);
	void cmdBindVertexBuffers(const std::vector<VulkanBuffer>& buffers, const std::vector<VkDeviceSize>& offsets);
	void cmdBindIndexBuffer(const VulkanBuffer& buffer, VkDeviceSize offset, VkIndexType indexType);

	void cmdCopyBuffer(const VulkanBuffer& source, const VulkanBuffer& destination, const std::vector<VkBufferCopy>& copyRegions);
	void cmdPushConstant(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);

	void cmdSetViewport(const VkViewport& viewport);
	void cmdSetScissor(VkRect2D scissor);

	void cmdDraw(uint32_t vertexCount, uint32_t firstVertex);
	void cmdDrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset);


	[[nodiscard]] uint32_t getID() const;

private:
	VulkanCommandBuffer() = default;
	VulkanCommandBuffer(VulkanDevice& device, VkCommandBuffer commandBuffer, bool isSecondary, uint32_t familyIndex, uint32_t threadID);

	VkCommandBuffer m_vkHandle = VK_NULL_HANDLE;
	uint32_t m_id = 0;

	bool m_isRecording = false;
	bool m_isSecondary = false;
	uint32_t m_familyIndex = 0;
	uint32_t m_threadID = 0;

	VulkanDevice* m_device;

	inline static uint32_t s_idCounter = 0;

	friend class VulkanDevice;
};


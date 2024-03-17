#pragma once
#include <map>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "vulkan_buffer.hpp"
#include "vulkan_gpu.hpp"
#include "vulkan_queues.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_shader.hpp"
#include <vulkan_image.hpp>
#include <vulkan_fence.hpp>

class VulkanQueue;

class VulkanDevice
{
public:

	void configureOneTimeQueue(QueueSelection queue);
	void initializeOneTimeCommandPool(uint32_t threadID);
	void initializeCommandPool(const QueueFamily& family, uint32_t threadID, bool secondary);
	uint32_t createCommandBuffer(const QueueFamily& family, uint32_t threadID, bool isSecondary);
	uint32_t createOneTimeCommandBuffer(uint32_t threadID);
	uint32_t getOrCreateCommandBuffer(const QueueFamily& family, uint32_t threadID, bool isSecondary);
	VulkanCommandBuffer& getCommandBuffer(uint32_t id, uint32_t threadID);
	void freeCommandBuffer(const VulkanCommandBuffer& commandBuffer, uint32_t threadID);
	void freeCommandBuffer(uint32_t id, uint32_t threadID);

	VkFramebuffer createFramebuffer(VkExtent3D size, const VulkanRenderPass& renderPass, const std::vector<VkImageView>& attachments);
	void freeFramebuffer(VkFramebuffer framebuffer);

	uint32_t createBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
	VulkanBuffer& getBuffer(uint32_t id);
	void freeBuffer(uint32_t id);
	void freeBuffer(const VulkanBuffer& buffer);

	uint32_t createImage(VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkImageCreateFlags flags);
	VulkanImage& getImage(uint32_t id);
	void freeImage(uint32_t id);
	void freeImage(const VulkanImage& image);

	void configureStagingBuffer(VkDeviceSize size, const QueueSelection& queue, bool forceAllowStagingMemory = false);
	void* mapStagingBuffer(VkDeviceSize size, VkDeviceSize offset);
	void unmapStagingBuffer();
	void dumpStagingBuffer(VulkanBuffer& buffer, VkDeviceSize size, VkDeviceSize offset, uint32_t threadID);
	void dumpStagingBuffer(VulkanBuffer& buffer, const std::vector<VkBufferCopy>& regions, uint32_t threadID);

	void disallowMemoryType(uint32_t type);
	void allowMemoryType(uint32_t type);

	uint32_t createRenderPass(const VulkanRenderPassBuilder& builder, VkRenderPassCreateFlags flags);
	VulkanRenderPass& getRenderPass(uint32_t id);
	void freeRenderPass(uint32_t id);
	void freeRenderPass(const VulkanRenderPass& renderPass);

	VkDescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
	void freeDescriptorSetLayout(VkDescriptorSetLayout layout);

	VkPipelineLayout createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges);
	void freePipelineLayout(VkPipelineLayout layout);

	uint32_t createShader(const std::string& filename, VkShaderStageFlagBits stage);
	VulkanShader& getShader(uint32_t id);
	void freeShader(uint32_t id);
	void freeShader(const VulkanShader& shader);
	void freeAllShaders();

	VkPipeline createPipeline(const VulkanPipelineBuilder& builder, VkPipelineLayout pipelineLayout, uint32_t renderPass, uint32_t subpass);
	void freePipeline(VkPipeline pipeline);

	VkSemaphore createSemaphore();
	void freeSemaphore(VkSemaphore semaphore);

	uint32_t createFence(bool signaled);
	VulkanFence& getFence(uint32_t id);
	void freeFence(uint32_t id);
	void freeFence(const VulkanFence& fence);

	void waitIdle();

	void free();

	[[nodiscard]] VulkanQueue getQueue(const QueueSelection& queueSelection) const;
	[[nodiscard]] VulkanGPU getGPU() const;
	[[nodiscard]] const VulkanMemoryAllocator& getMemoryAllocator() const;
	[[nodiscard]] VkSemaphore getStagingBufferSemaphore() const;

private:
	[[nodiscard]] VkDeviceMemory getMemoryHandle(uint32_t chunk) const;

	VulkanBuffer _createBuffer(VkDeviceSize size, VkBufferUsageFlags usage);

	struct ThreadCommandInfo
	{
		struct CommandPoolInfo
		{
			VkCommandPool pool = VK_NULL_HANDLE;
			VkCommandPool secondaryPool = VK_NULL_HANDLE;
		};

		VkCommandPool oneTimePool = VK_NULL_HANDLE;
		std::map<uint32_t, CommandPoolInfo> commandPools;
	};

	struct StagingBufferInfo
	{
		VulkanBuffer stagingBuffer{};
		QueueSelection queue{};
	} m_stagingBufferInfo;

	VulkanDevice() = default;
	VulkanDevice(VulkanGPU pDevice, VkDevice device);

	VkDevice m_vkHandle;

	VulkanGPU m_physicalDevice;

	std::map<uint32_t, ThreadCommandInfo> m_threadCommandInfos;
	std::vector<VkFramebuffer> m_framebuffers;
	std::vector<VulkanBuffer> m_buffers;
	std::unordered_map<uint32_t, std::vector<VulkanCommandBuffer>> m_commandBuffers;
	std::vector<VulkanRenderPass> m_renderPasses;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<VkPipelineLayout> m_pipelineLayouts;
	std::vector<VulkanShader> m_shaders;
	std::vector<VkPipeline> m_pipelines;
	std::vector<VulkanImage> m_images;
	std::vector<VkSemaphore> m_semaphores;
	std::vector<VulkanFence> m_fences;

	VulkanMemoryAllocator m_memoryAllocator;
	VkSemaphore m_stagingSemaphore = VK_NULL_HANDLE;
	QueueSelection m_oneTimeQueue{UINT32_MAX, UINT32_MAX};

	friend class VulkanContext;
	friend class SDLWindow;
	friend class VulkanGPU;
	friend class VulkanResource;
	friend class VulkanMemoryAllocator;
	friend class VulkanBuffer;
	friend class VulkanRenderPass;
	friend class VulkanImage;
	friend class VulkanFence;
};

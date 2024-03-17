#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

class VulkanDevice;

enum AttachmentType
{
	COLOR,
	DEPTH_STENCIL,
	INPUT,
	RESOLVE,
	PRESERVE
};

class VulkanRenderPassBuilder
{
public:
	struct AttachmentReference
	{
		AttachmentType type;
		uint32_t attachment;
		VkImageLayout layout;
	};

	VulkanRenderPassBuilder& addAttachment(const VkAttachmentDescription& attachment);
	VulkanRenderPassBuilder& addSubpass(VkPipelineBindPoint bindPoint, const std::vector<AttachmentReference>& attachments, VkSubpassDescriptionFlags flags);
	VulkanRenderPassBuilder& addDependency(const VkSubpassDependency& dependency);

	static VkAttachmentDescription createAttachment(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkImageLayout initialLayout, VkImageLayout finalLayout);


private:
	struct SubpassInfo
	{
		VkPipelineBindPoint bindPoint;
		VkSubpassDescriptionFlags flags;
		std::vector<VkAttachmentReference> colorAttachments;
		std::vector<VkAttachmentReference> resolveAttachments;
		std::vector<VkAttachmentReference> inputAttachments;
		VkAttachmentReference depthStencilAttachment;
		std::vector<uint32_t> preserveAttachments;
		bool hasDepthStencilAttachment = false;
	};

	std::vector<VkAttachmentDescription> m_attachments;
	std::vector<SubpassInfo> m_subpasses;
	std::vector<VkSubpassDependency> m_dependencies;

	friend class VulkanDevice;
};

class VulkanRenderPass
{
	[[nodiscard]] uint32_t getID() const;

private:
	void free();
	VulkanRenderPass() = default;
	VulkanRenderPass(VulkanDevice* device, VkRenderPass renderPass);

	VkRenderPass m_vkHandle = VK_NULL_HANDLE;
	VulkanDevice* m_device = nullptr;

	uint32_t m_id = 0;

	inline static uint32_t s_idCounter = 0;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};


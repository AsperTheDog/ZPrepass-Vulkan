#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <shaderc/shaderc.hpp>

class VulkanShader
{
public:
	static [[nodiscard]] shaderc_shader_kind getKindFromStage(VkShaderStageFlagBits stage);

	[[nodiscard]] uint32_t getID() const;

private:
	struct Result
	{
		bool success = false;
		std::vector<uint32_t> code;
		std::string error;
	};

	VulkanShader() = default;
	explicit VulkanShader(VkShaderModule handle, VkShaderStageFlagBits stage);

	static std::string readFile(std::string_view p_filename);
	static [[nodiscard]] Result compileFile(std::string_view p_source_name, shaderc_shader_kind p_kind, std::string_view p_source, bool p_optimize);

	uint32_t m_id = 0;

	VkShaderModule m_vkHandle = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

	inline static uint32_t m_idCounter = 0;

	friend class VulkanDevice;
	friend class VulkanPipeline;
	friend struct VulkanPipelineBuilder;
};



#include <iostream>
#include <stdexcept>
#include <array>
#include <bit>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "logger.hpp"
#include "sdl_window.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


SDLWindow window;

uint32_t deviceID = UINT32_MAX;

glm::mat4 viewMatrix;
glm::mat4 projMatrix;
std::vector<glm::mat4> modelMatrices;
std::vector<glm::vec3> modelColors;

glm::mat4 getMVPMat(const uint32_t model) { return projMatrix * viewMatrix * modelMatrices[model]; }

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 texCoord;
	glm::vec3 normal;

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && texCoord == other.texCoord && normal == other.normal;
	}
};

template<> struct std::hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const noexcept
	{
		return hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;

	[[nodiscard]] glm::mat4 getMVP() const
	{
		return proj * view * model;
	}
};

std::vector<Vertex> vertices;
std::vector<uint32_t> indices;

VulkanGPU getCorrectGPU()
{
	const std::vector<VulkanGPU> gpus = VulkanContext::getGPUs();
	for (const VulkanGPU& gpu : gpus)
	{
		const VkPhysicalDeviceProperties properties = gpu.getProperties();
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			return gpu;
		}
	}
	throw std::runtime_error("No discrete GPU found");
}

void loadModel(std::string_view filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.data())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const tinyobj::shape_t& shape : shapes) {
        for (const tinyobj::index_t& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

			vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            if (!uniqueVertices.contains(vertex)) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

uint32_t createRenderPass()
{
	const VkFormat depthFormat = VulkanContext::getDevice(deviceID).getGPU().findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	VulkanRenderPassBuilder builder{};

	const VkAttachmentDescription colorAttachment = VulkanRenderPassBuilder::createAttachment(window.getSwapchainImageFormat().format, 
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	builder.addAttachment(colorAttachment);

	const VkAttachmentDescription depthAttachment = VulkanRenderPassBuilder::createAttachment(depthFormat,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	builder.addAttachment(depthAttachment);

	std::vector<VulkanRenderPassBuilder::AttachmentReference> depthSubpassRefs;
	depthSubpassRefs.push_back({DEPTH_STENCIL, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
	builder.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, depthSubpassRefs, 0);

	std::vector<VulkanRenderPassBuilder::AttachmentReference> colorSubpassRefs;
	colorSubpassRefs.push_back({COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
	colorSubpassRefs.push_back({DEPTH_STENCIL, 1, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL});
	builder.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colorSubpassRefs, 0);

	VkSubpassDependency	dependency;
	dependency.srcSubpass = 0;
	dependency.dstSubpass = 1;
	dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	dependency.dependencyFlags = 0;
	builder.addDependency(dependency);

	return VulkanContext::getDevice(deviceID).createRenderPass(builder, 0);
}

std::tuple<uint32_t, uint32_t, uint32_t> createGraphicsPipelines(const uint32_t renderPassID)
{
	VkPushConstantRange pushConstantVertex{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)};
	VkPushConstantRange pushConstantFragment{VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec3)};
	const uint32_t layout = VulkanContext::getDevice(deviceID).createPipelineLayout({}, {pushConstantVertex, pushConstantFragment});

	const uint32_t vertexDepthShader = VulkanContext::getDevice(deviceID).createShader("shaders/depth.vert", VK_SHADER_STAGE_VERTEX_BIT);
	const uint32_t vertexColorShader = VulkanContext::getDevice(deviceID).createShader("shaders/color.vert", VK_SHADER_STAGE_VERTEX_BIT);
	const uint32_t fragmentColorShader = VulkanContext::getDevice(deviceID).createShader("shaders/color.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	VulkanBinding binding{0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(Vertex)};
	binding.addAttribDescription(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
	binding.addAttribDescription(VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord));
	binding.addAttribDescription(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VulkanPipelineBuilder builder{&VulkanContext::getDevice(deviceID)};

	builder.addVertexBinding(binding);
	builder.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
	builder.setViewportState(1, 1);
	builder.setRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	builder.setMultisampleState(VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f);
	builder.setDepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	builder.addColorBlendAttachment(colorBlendAttachment);
	builder.setColorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, {0.0f, 0.0f, 0.0f, 0.0f});
	builder.setDynamicState({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});
	builder.addShaderStage(vertexDepthShader);
	const uint32_t depthPipeline = VulkanContext::getDevice(deviceID).createPipeline(builder, layout, renderPassID, 0);

	builder.setDepthStencilState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_EQUAL);
	builder.resetShaderStages();
	builder.addShaderStage(vertexColorShader);
	builder.addShaderStage(fragmentColorShader);
	const uint32_t colorPipeline = VulkanContext::getDevice(deviceID).createPipeline(builder, layout, renderPassID, 1);

	return {depthPipeline, colorPipeline, layout};
}

std::pair<uint32_t, VkImageView> createDepthImage(const VkFormat depthFormat)
{
	const VkExtent2D extent = window.getSwapchainExtent();
	uint32_t depthImage = VulkanContext::getDevice(deviceID).createImage(VK_IMAGE_TYPE_2D, depthFormat, {extent.width, extent.height, 1}, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0);
	VulkanContext::getDevice(deviceID).getImage(depthImage).allocateFromFlags({VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, false});
	VulkanImage& depthImageObj = VulkanContext::getDevice(deviceID).getImage(depthImage);
	VkImageView depthImageView = depthImageObj.createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	return {depthImage, depthImageView};
}

uint32_t createFramebuffer(const uint32_t renderPassID, const VkImageView colorAttachment, const VkImageView depthAttachment)
{
	const std::vector<VkImageView> attachments{colorAttachment, depthAttachment};
	const VkExtent2D extent = window.getSwapchainExtent();
	return VulkanContext::getDevice(deviceID).createFramebuffer({extent.width, extent.height, 1}, VulkanContext::getDevice(deviceID).getRenderPass(renderPassID), attachments);
}

void recordFramebuffer(const uint32_t commandbufferID, const uint32_t renderPassID, const uint32_t framebufferID, const uint32_t depthPipelineID, const uint32_t colorPipelineID, const uint32_t layoutID, const uint32_t objectBufferID)
{
	Logger::pushContext("Command buffer recording");

	std::vector<VkClearValue> clearValues{2};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

	VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(window.getSwapchainExtent().width);
    viewport.height = static_cast<float>(window.getSwapchainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

	VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = window.getSwapchainExtent();


	VulkanCommandBuffer& graphicsBuffer = VulkanContext::getDevice(deviceID).getCommandBuffer(commandbufferID, 0);
	graphicsBuffer.reset();
	graphicsBuffer.beginRecording(0);

	graphicsBuffer.cmdBeginRenderPass(renderPassID, framebufferID, window.getSwapchainExtent(), clearValues);

		graphicsBuffer.cmdBindVertexBuffer(objectBufferID, 0);
		graphicsBuffer.cmdBindIndexBuffer(objectBufferID, vertices.size() * sizeof(vertices[0]), VK_INDEX_TYPE_UINT32);

		graphicsBuffer.cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, depthPipelineID);
		graphicsBuffer.cmdSetViewport(viewport);
		graphicsBuffer.cmdSetScissor(scissor);

		for (int i = static_cast<int>(modelMatrices.size()) - 1; i >= 0 ; --i)
		{
			const glm::mat4 mvpMat = getMVPMat(i);
			graphicsBuffer.cmdPushConstant(layoutID, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvpMat);
			graphicsBuffer.cmdDrawIndexed(static_cast<uint32_t>(indices.size()), 0, 0);
		}

		graphicsBuffer.cmdNextSubpass();

		graphicsBuffer.cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, colorPipelineID);
		graphicsBuffer.cmdSetViewport(viewport);
		graphicsBuffer.cmdSetScissor(scissor);

		for (int i = static_cast<int>(modelMatrices.size()) - 1; i >= 0 ; --i)
		{
			const glm::mat4 mvpMat = getMVPMat(i);
			const glm::vec3 modelColor = modelColors[i];
			graphicsBuffer.cmdPushConstant(layoutID, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvpMat);
			graphicsBuffer.cmdPushConstant(layoutID, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec3), &modelColor);
			graphicsBuffer.cmdDrawIndexed(static_cast<uint32_t>(indices.size()), 0, 0);
		}

	graphicsBuffer.cmdEndRenderPass();
	graphicsBuffer.endRecording();

	Logger::popContext();
}

int main()
{
	Logger::setRootContext("Initialization");

	// Create window and Vulkan context
	window = SDLWindow{"Test", 1920, 1080};
	VulkanContext::init(VK_API_VERSION_1_3, true, window.getRequiredVulkanExtensions());
	window.createSurface();

	// Select GPU and get Queue Families
	const VulkanGPU selectedGPU = getCorrectGPU();
	const GPUQueueStructure queueStructure = selectedGPU.getQueueFamilies();

	std::cout << "\n*************************************************************************\n"
			  << "******************************* Structure *******************************\n"
			  << "*************************************************************************\n\n"
			  << queueStructure.toString();

	const QueueFamily graphicsQueueFamily = queueStructure.findQueueFamily(VK_QUEUE_GRAPHICS_BIT);
	const QueueFamily presentQueueFamily = queueStructure.findPresentQueueFamily(window.getSurface());
	const QueueFamily transferQueueFamily = queueStructure.findQueueFamily(VK_QUEUE_TRANSFER_BIT);

	// Select Queue Families and assign queues
	QueueFamilySelector selector{queueStructure};
	selector.selectQueueFamily(graphicsQueueFamily, QueueFamilyTypeBits::GRAPHICS);
	selector.selectQueueFamily(presentQueueFamily, QueueFamilyTypeBits::PRESENT);
	const QueueSelection graphicsQueuePos = selector.getOrAddQueue(graphicsQueueFamily, 1.0);
	const QueueSelection presentQueuePos = selector.getOrAddQueue(presentQueueFamily, 1.0);
	const QueueSelection transferQueuePos = selector.addQueue(transferQueueFamily, 1.0);

	// Create device and memory allocation system
	deviceID = VulkanContext::createDevice(selectedGPU, selector, {VK_KHR_SWAPCHAIN_EXTENSION_NAME}, {});
	VulkanDevice& device = VulkanContext::getDevice(deviceID);

	std::cout << "\n*************************************************************************\n"
	          <<   "*************************** Memory Properties ***************************\n"
	          <<   "*************************************************************************\n\n"
			  << device.getMemoryAllocator().getMemoryStructure().toString();

	std::cout << "\n*************************************************************************\n"
			  <<   "*************************************************************************\n"
			  <<   "*************************************************************************\n\n";

	window.createSwapchain(deviceID, {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});

	device.configureOneTimeQueue(transferQueuePos);
	uint32_t graphicsBufferID = device.createCommandBuffer(graphicsQueueFamily, 0, false);

	uint32_t renderPassID = createRenderPass();
	const auto [depthPipeline, colorPipeline, pipelineLayout] = createGraphicsPipelines(renderPassID);

	// Configure buffers
	device.configureStagingBuffer(5LL * 1024 * 1024, transferQueuePos);

	loadModel("models/stanfordDragon.obj");
	uint32_t objectBufferID = device.createBuffer(sizeof(vertices[0]) * vertices.size() + sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	VulkanBuffer& objectBuffer = device.getBuffer(objectBufferID);
	objectBuffer.allocateFromFlags({VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, false});

	{
		void* dataPtr = device.mapStagingBuffer(objectBuffer.getSize(), 0);
		memcpy(dataPtr, vertices.data(), sizeof(vertices[0]) * vertices.size());
		memcpy(static_cast<char*>(dataPtr) + sizeof(vertices[0]) * vertices.size(), indices.data(), sizeof(indices[0]) * indices.size());
		device.dumpStagingBuffer(objectBufferID, objectBuffer.getSize(), 0, 0);
	}

	// Configure depth buffer
	const VkFormat depthFormat = device.getGPU().findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	auto [depthImage, depthImageView] = createDepthImage(depthFormat);

	// Create frame buffers
	std::vector<uint32_t> framebuffers{};
	framebuffers.resize(window.getImageCount());
	for (uint32_t i = 0; i < window.getImageCount(); i++)
		framebuffers[i] = createFramebuffer(renderPassID, window.getImageView(i), depthImageView);

	// Create sync objects
	uint32_t imageAvailableSemaphoreID = device.createSemaphore();
	uint32_t renderFinishedSemaphoreID = device.createSemaphore();
	uint32_t inFlightFenceID = device.createFence(true);
	VulkanFence& inFlightFence = device.getFence(inFlightFenceID);

	VulkanQueue graphicsQueue = device.getQueue(graphicsQueuePos);
	VulkanQueue presentQueue = device.getQueue(presentQueuePos);
	VulkanCommandBuffer& graphicsBuffer = device.getCommandBuffer(graphicsBufferID, 0);

	// Configure push constant data
	{
		Logger::pushContext("Camera and model config");

		viewMatrix = glm::lookAt(glm::vec3(0.0f, -120.0f, 150.0f), glm::vec3(0.0f, -80.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		float aspectRatio = static_cast<float>(window.getSwapchainExtent().width) / static_cast<float>(window.getSwapchainExtent().height);
		projMatrix = glm::perspective(glm::radians(70.0f), aspectRatio, 0.1f, 500.0f);

		for (uint32_t i = 0; i < 5; i++)
		{
			float fi = static_cast<float>(i);
			modelMatrices.emplace_back(1.0f);
			modelMatrices.back() = glm::translate(modelMatrices.back(), glm::vec3(10.0f + -30.0f * fi, -20.0f * fi, -30.0f * fi));
			modelMatrices.back() = glm::rotate(modelMatrices.back(), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrices.back() = glm::scale(modelMatrices.back(), {1.0, -1.0, 1.0});

			float value = fi / 5.0f;

			modelColors.emplace_back(value, 1.0f - value, 1.0f);
		}

		Logger::popContext();
	}

	// Main loop
	uint64_t frameCounter = 0;
	Logger::setRootContext("Frame" + std::to_string(frameCounter));
	while (!window.shouldClose())
	{
		window.pollEvents();

		inFlightFence.wait();

		if (window.getAndResetSwapchainRebuildFlag())
		{
			Logger::pushContext("Swapchain resources rebuild");
			device.freeImage(depthImage);
			std::tie(depthImage, depthImageView) = createDepthImage(depthFormat);

			for (uint32_t i = 0; i < window.getImageCount(); i++)
			{
				device.freeFramebuffer(framebuffers[i]);
				framebuffers[i] = createFramebuffer(renderPassID, window.getImageView(i), depthImageView);
			}

			float aspectRatio = static_cast<float>(window.getSwapchainExtent().width) / static_cast<float>(window.getSwapchainExtent().height);
			projMatrix = glm::perspective(glm::radians(70.0f), aspectRatio, 0.1f, 500.0f);

			Logger::popContext();
		}

		uint32_t nextImage = window.acquireNextImage(imageAvailableSemaphoreID, nullptr);
		if (nextImage == UINT32_MAX)
		{
			inFlightFence.reset();
			frameCounter++;
			continue;
		}

		inFlightFence.reset();

		recordFramebuffer(graphicsBufferID, renderPassID, framebuffers[nextImage], depthPipeline, colorPipeline, pipelineLayout, objectBufferID);

		graphicsBuffer.submit(graphicsQueue, {{imageAvailableSemaphoreID, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}}, {renderFinishedSemaphoreID}, inFlightFenceID);
		window.present(presentQueue, nextImage, renderFinishedSemaphoreID);

		frameCounter++;
	}

	device.waitIdle();

	// Free resources
	Logger::setRootContext("Resource cleanup");
	window.free();
	VulkanContext::free();
	return 0;
}
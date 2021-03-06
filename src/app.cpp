#include "app.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "gfx.h"
#include "string_utils.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const int WIDTH = 800;
const int HEIGHT = 600;

static int s_width = WIDTH;
static int s_height = HEIGHT;

static GFX::Shader vertShader;
static GFX::Shader fragShader;
static GFX::Shader screenVertShader;
static GFX::Shader screenFragShader;

static GFX::Pipeline pipeline;
static GFX::Buffer vertexBuffer;
static GFX::Buffer indexBuffer;
static GFX::Buffer uniformBuffer;
static GFX::UniformLayout uniformLayout;
static GFX::Uniform uniform;
static GFX::Image image;
static GFX::Sampler sampler;
static GFX::RenderPass renderPass;

static GFX::UniformLayout screenQuadUniformLayout;
static GFX::Uniform screenQuadUniform;

static GFX::Pipeline screenQuadPipeline;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
};

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

struct UniformBufferObject 
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

void App::Run()
{
	Init();

	MainLoop();

	CleanUp();
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) 
{
	s_width = width;
	s_height = height;

	spdlog::info("Window Resize");
	GFX::Resize(width, height);
	GFX::ResizeRenderPass(renderPass, width, height);

	GFX::DestroyUniform(screenQuadUniform);

	GFX::UniformDescription screenQuadUniformDesc = {};
	screenQuadUniformDesc.SetUniformLayout(screenQuadUniformLayout);
	screenQuadUniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);
	screenQuadUniformDesc.AddSampledAttachmentAttribute(0, renderPass, 1, sampler);

	screenQuadUniform = GFX::CreateUniform(screenQuadUniformDesc);
}

void App::Init()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	m_window = glfwCreateWindow(WIDTH, HEIGHT, "mo-gfx", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	GFX::InitialDescription initDesc = {};
	initDesc.debugMode = true;
	initDesc.window = m_window;

	GFX::Init(initDesc);

	GFX::BufferDescription vertexBufferDescription = {};
	vertexBufferDescription.size = sizeof(Vertex) * vertices.size();
	vertexBufferDescription.storageMode = GFX::BufferStorageMode::Static;
	vertexBufferDescription.usage = GFX::BufferUsage::VertexBuffer;

	vertexBuffer = GFX::CreateBuffer(vertexBufferDescription);
	GFX::UpdateBuffer(vertexBuffer, 0, vertexBufferDescription.size, (void*)vertices.data());

	GFX::BufferDescription indexBufferDescription = {};
	indexBufferDescription.size = sizeof(uint16_t) * indices.size();
	indexBufferDescription.storageMode = GFX::BufferStorageMode::Static;
	indexBufferDescription.usage = GFX::BufferUsage::IndexBuffer;

	indexBuffer = GFX::CreateBuffer(indexBufferDescription);
	GFX::UpdateBuffer(indexBuffer, 0, indexBufferDescription.size, (void*)indices.data());

	GFX::BufferDescription uniformBufferDescription = {};
	uniformBufferDescription.size = GFX::UniformAlign(sizeof(UniformBufferObject)) + GFX::UniformAlign(sizeof(float));
	uniformBufferDescription.storageMode = GFX::BufferStorageMode::Dynamic;
	uniformBufferDescription.usage = GFX::BufferUsage::UniformBuffer;

	uniformBuffer = GFX::CreateBuffer(uniformBufferDescription);

	GFX::ShaderDescription vertDesc = {};
	vertDesc.name = "default";
	vertDesc.codes = StringUtils::ReadFile("default.vert");
	vertDesc.stage = GFX::ShaderStage::Vertex;

	GFX::ShaderDescription fragDesc = {};
	fragDesc.name = "default";
	fragDesc.codes = StringUtils::ReadFile("default.frag");
	fragDesc.stage = GFX::ShaderStage::Fragment;

	vertShader = GFX::CreateShader(vertDesc);
	fragShader = GFX::CreateShader(fragDesc);

	GFX::VertexBindings vertexBindings = {};
	vertexBindings.AddAttribute(0, offsetof(Vertex, pos), GFX::ValueType::Float32x3);
	vertexBindings.AddAttribute(1, offsetof(Vertex, color), GFX::ValueType::Float32x3);
	vertexBindings.AddAttribute(2, offsetof(Vertex, texCoord), GFX::ValueType::Float32x2);
	vertexBindings.SetStrideSize(sizeof(Vertex));
	vertexBindings.SetBindingType(GFX::BindingType::Vertex);
	vertexBindings.SetBindingPosition(0);

	GFX::UniformLayoutDescription uniformLayoutDesc = {};
	uniformLayoutDesc.AddUniformBinding(0, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Vertex, 1);
	uniformLayoutDesc.AddUniformBinding(1, GFX::UniformType::UniformBuffer, GFX::ShaderStage::Vertex, 1);
	uniformLayoutDesc.AddUniformBinding(2, GFX::UniformType::SampledImage, GFX::ShaderStage::Fragment, 1);

	uniformLayout = GFX::CreateUniformLayout(uniformLayoutDesc);

	GFX::UniformBindings uniformBindings = {};
	uniformBindings.AddUniformLayout(uniformLayout);

	CreateRenderPass();

	GFX::GraphicsPipelineDescription pipelineDesc = {};
	pipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
	pipelineDesc.shaders.push_back(vertShader);
	pipelineDesc.shaders.push_back(fragShader);
	pipelineDesc.vertexBindings = vertexBindings;
	pipelineDesc.uniformBindings = uniformBindings;
	pipelineDesc.renderPass = renderPass;
	pipelineDesc.enableDepthTest = true;
	pipelineDesc.subpass = 0;

	pipeline = GFX::CreatePipeline(pipelineDesc);

	GFX::ShaderDescription screenQuadVertDesc = {};
	screenQuadVertDesc.name = "default";
	screenQuadVertDesc.codes = StringUtils::ReadFile("screen_quad.vert");
	screenQuadVertDesc.stage = GFX::ShaderStage::Vertex;

	GFX::ShaderDescription screenQuadFragDesc = {};
	screenQuadFragDesc.name = "default";
	screenQuadFragDesc.codes = StringUtils::ReadFile("screen_quad_test.frag");
	screenQuadFragDesc.stage = GFX::ShaderStage::Fragment;

	screenVertShader = GFX::CreateShader(screenQuadVertDesc);
	screenFragShader = GFX::CreateShader(screenQuadFragDesc);

	GFX::UniformLayoutDescription screenQuadUniformLayoutDesc = {};
	screenQuadUniformLayoutDesc.AddUniformBinding(0, 
		// GFX::UniformType::InputAttachment, 
		GFX::UniformType::SampledImage,
		GFX::ShaderStage::Fragment, 
		1);

	screenQuadUniformLayout = GFX::CreateUniformLayout(screenQuadUniformLayoutDesc);

	GFX::UniformBindings screenQuadUniformBindings = {};
	screenQuadUniformBindings.AddUniformLayout(screenQuadUniformLayout);

	GFX::GraphicsPipelineDescription screenQuadPipelineDesc = {};
	screenQuadPipelineDesc.primitiveTopology = GFX::PrimitiveTopology::TriangleList;
	screenQuadPipelineDesc.renderPass = renderPass;
	screenQuadPipelineDesc.shaders.push_back(screenVertShader);
	screenQuadPipelineDesc.shaders.push_back(screenFragShader);
	screenQuadPipelineDesc.enableDepthTest = false;
	screenQuadPipelineDesc.uniformBindings = screenQuadUniformBindings;
	screenQuadPipelineDesc.subpass = 1;

	screenQuadPipeline = GFX::CreatePipeline(screenQuadPipelineDesc);

	LoadTexture();

	GFX::SamplerDescription samplerDesc = {};
	samplerDesc.minFilter = GFX::FilterMode::Linear;
	samplerDesc.magFilter = GFX::FilterMode::Linear;
	samplerDesc.wrapU = GFX::WrapMode::ClampToEdge;
	samplerDesc.wrapV = GFX::WrapMode::ClampToEdge;

	sampler = GFX::CreateSampler(samplerDesc);

	GFX::UniformDescription uniformDesc = {};
	uniformDesc.SetUniformLayout(uniformLayout);
	uniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);
	uniformDesc.AddBufferAttribute(0, uniformBuffer, 0, sizeof(UniformBufferObject));
	uniformDesc.AddBufferAttribute(
		1,
		uniformBuffer,
		GFX::UniformAlign(
			sizeof(UniformBufferObject)),
		sizeof(float)
	);
	uniformDesc.AddImageAttribute(2, image, sampler);

	uniform = GFX::CreateUniform(uniformDesc);

	GFX::UniformDescription screenQuadUniformDesc = {};
	screenQuadUniformDesc.SetUniformLayout(screenQuadUniformLayout);
	screenQuadUniformDesc.SetStorageMode(GFX::UniformStorageMode::Dynamic);
	screenQuadUniformDesc.AddSampledAttachmentAttribute(0, renderPass, 1, sampler);

	screenQuadUniform = GFX::CreateUniform(screenQuadUniformDesc);
}

void App::CreateRenderPass()
{
	// Render Pass
	GFX::RenderPassDescription renderPassDescription = {};
	renderPassDescription.width = s_width;
	renderPassDescription.height = s_height;

	GFX::AttachmentDescription swapChainAttachment = {};
	swapChainAttachment.format = GFX::Format::SWAPCHAIN;
	swapChainAttachment.width = s_width;
	swapChainAttachment.height = s_height;
	swapChainAttachment.type = GFX::AttachmentType::Present;
	swapChainAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	swapChainAttachment.storeAction = GFX::AttachmentStoreAction::Store;

	GFX::AttachmentDescription hdrAttachment = {};
	hdrAttachment.format = GFX::Format::R16G16B16A16F;
	hdrAttachment.width = s_width;
	hdrAttachment.height = s_height;
	hdrAttachment.type = GFX::AttachmentType::Color;
	hdrAttachment.loadAction = GFX::AttachmentLoadAction::Clear;
	hdrAttachment.storeAction = GFX::AttachmentStoreAction::DontCare;

	GFX::AttachmentDescription depthAttachment = {};
	depthAttachment.format = GFX::Format::DEPTH_24UNORM_STENCIL_8INT;
	depthAttachment.width = s_width;
	depthAttachment.height = s_height;
	depthAttachment.type = GFX::AttachmentType::DepthStencil;
	depthAttachment.loadAction = GFX::AttachmentLoadAction::Clear;

	// attachment 0, swap chain
	renderPassDescription.attachments.push_back(swapChainAttachment);
	// attachment 1, hdr
	renderPassDescription.attachments.push_back(hdrAttachment);
	// attachment 2, depth
	renderPassDescription.attachments.push_back(depthAttachment);

	GFX::SubPassDescription subPassHdr = {};
	subPassHdr.pipelineType = GFX::PipelineType::Graphics;
	subPassHdr.colorAttachments.push_back(1);
	subPassHdr.SetDepthStencilAttachment(2);

	GFX::SubPassDescription subPassSwapChain = {};
	subPassSwapChain.pipelineType = GFX::PipelineType::Graphics;
	subPassSwapChain.colorAttachments.push_back(0);
	subPassSwapChain.inputAttachments.push_back(1);
	subPassSwapChain.SetDepthStencilAttachment(2);

	renderPassDescription.subpasses.push_back(subPassHdr);
	renderPassDescription.subpasses.push_back(subPassSwapChain);

	GFX::DependencyDescription dependencyDesc = {};
	dependencyDesc.srcSubpass = 0;
	dependencyDesc.dstSubpass = 1;
	dependencyDesc.srcStage = GFX::PipelineStage::ColorAttachmentOutput;
	dependencyDesc.dstStage = GFX::PipelineStage::FragmentShader;
	dependencyDesc.srcAccess = GFX::Access::ColorAttachmentWrite;
	dependencyDesc.dstAccess = GFX::Access::ShaderRead;

	renderPassDescription.dependencies.push_back(dependencyDesc);

	renderPass = GFX::CreateRenderPass(renderPassDescription);
}

void App::LoadTexture()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	GFX::ImageDescription imageDescription = {};
	imageDescription.format = GFX::Format::R8G8B8A8;
	imageDescription.width = texWidth;
	imageDescription.height = texHeight;
	imageDescription.depth = 1;
	imageDescription.readOrWriteByCPU = false;
	imageDescription.usage = GFX::ImageUsage::SampledImage;
	imageDescription.type = GFX::ImageType::Image2D;
	imageDescription.sampleCount = GFX::ImageSampleCount::Sample1;

	image = GFX::CreateImage(imageDescription);
	GFX::UpdateImageMemory(image, pixels, sizeof(stbi_uc) * texWidth * texHeight * 4);

	STBI_FREE(pixels);
}

void App::MainLoop()
{
	while (!glfwWindowShouldClose(m_window)) 
	{
		glfwPollEvents();

		if (GFX::BeginFrame())
		{
			GFX::BeginRenderPass(renderPass, 0, 0, s_width, s_height);

			GFX::ApplyPipeline(pipeline);
			
			float time = (float)glfwGetTime();
			UniformBufferObject ubo = {};
			ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.proj = glm::perspective(glm::radians(45.0f), (float)s_width/(float)s_height, 0.1f, 10.0f);

			GFX::UpdateUniformBuffer(uniform, 0, &ubo);
			GFX::UpdateUniformBuffer(uniform, 1, &time);

			GFX::BindIndexBuffer(indexBuffer, 0, GFX::IndexType::UInt16);
			GFX::BindVertexBuffer(vertexBuffer, 0);
			GFX::BindUniform(uniform, 0);

			GFX::SetViewport(0, 0, s_width, s_height);
			GFX::SetScissor(0, 0, s_width, s_height);

			GFX::DrawIndexed(indices.size(), 1, 0);

			GFX::NextRenderPass();

			GFX::ApplyPipeline(screenQuadPipeline);
			GFX::BindUniform(screenQuadUniform, 0);
			GFX::Draw(3, 1, 0, 0);

			GFX::EndRenderPass();

			GFX::EndFrame();
		}
	}
}

void App::CleanUp()
{
	GFX::DestroyRenderPass(renderPass);

	GFX::DestroySampler(sampler);
	GFX::DestroyImage(image);

	GFX::DestroyUniform(uniform);
	GFX::DestroyUniformLayout(uniformLayout);

	GFX::DestroyUniform(screenQuadUniform);
	GFX::DestroyUniformLayout(screenQuadUniformLayout);

	GFX::DestroyBuffer(vertexBuffer);
	GFX::DestroyBuffer(indexBuffer);
	GFX::DestroyBuffer(uniformBuffer);

	GFX::DestroyPipeline(pipeline);

	GFX::DestroyShader(vertShader);
	GFX::DestroyShader(fragShader);

	GFX::DestroyPipeline(screenQuadPipeline);

	GFX::DestroyShader(screenVertShader);
	GFX::DestroyShader(screenFragShader);

	GFX::Shutdown();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}
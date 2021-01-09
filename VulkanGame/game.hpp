#pragma once

#ifndef GAME_HPP
#define GAME_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "utils.hpp"
#include "constants.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <set>
#include <vector>

class Game {
public:
	/*
	 * @brief Runs the Vulkan game engine.
	 */
	void run();

private:
	// window object
	GLFWwindow *window;

	// vulkan instance
	VkInstance instance;

	// debug messenger for validation layers
	VkDebugUtilsMessengerEXT debugMessenger;

	// device to use for graphics processing;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	// logical device to interact with physical device
	VkDevice device;

	// surface to present rendered images to
	VkSurfaceKHR surface;

	// handle to the graphics queue on logical device
	VkQueue graphicsQueue;

	// handle to presentation of rendered images queue
	VkQueue presentQueue;

	// actual swapchain
	VkSwapchainKHR swapChain;

	std::vector<VkImage> swapchainImages;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

	// image views from the swapchain and their framebuffers
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers;

	// render pass for pipeline
	VkRenderPass renderPass;

	// layout for descriptor set
	VkDescriptorSetLayout descSetLayout;

	// pipeline layout
	VkPipelineLayout pipelineLayout;

	// final pipeline
	VkPipeline graphicsPipeline;

	// command pool manages memory used to store buffers
	VkCommandPool commandPool;

	// vertex buffer for vertex information
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	// index buffer to avoid reuse of vertices
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	// uniform buffers
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	// descriptor pool and sets
	VkDescriptorPool descPool;
	std::vector<VkDescriptorSet> descSets;

	// command buffers that use command pool
	std::vector<VkCommandBuffer> commandBuffers;

	// semaphores
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	// fences
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	size_t currentFrame = 0;

	// check for framebuffer being resized
	bool framebufferResized = false;

	// uses model-view-projection to allow vertices to change each frame, if necessary
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	// vertices to draw on the screen
	const std::vector<Utils::Vertex> vertices = {
		{{-0.25f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.25f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.75f, 0.5f}, {1.0f, 1.0f, 1.0f}},
		{{-0.75f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	// indices for the vertices (using uint16_t because we have fewer than 65535 unique vertices)
	const std::vector<uint16_t> vertexIndices = {
		0, 1, 2, 2, 3, 0
	};

	/*
	 * @brief Called when GLFW detects that the framebuffer has been resized.
	 */
	static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

	/*
	 * @brief Initializes a window.
	 */
	void initWindow();

	/*
	 * @brief Creates a Vulkan instance and checks if the GLFW required extensions are
	 * available.
	 */
	void createInstance();

	/*
	 * @brief Sets up a messenger to display validation layer debug messages.
	 */
	void setupDebugMessenger();

	/*
	 * @brief Creates a surface to present rendered images to.
	 */
	void createSurface();

	/*
	 * @brief Selects a graphics card that supports the necessary features.
	 */
	void pickPhysicalDevice();

	/*
	 * @brief Creates a logical device that will interact with physical device.
	 */
	void createLogicalDevice();

	/*
	 * @brief Creates the swapchain for Vulkan to use.
	 */
	void createSwapchain();

	/*
	 * @brief Creates the image views for the swapchain.
	 */
	void createImageViews();

	/*
	 * @brief Creates a render pass that specifies information about the framebuffer
	 * attachments and how many colour and depth buffers there will be.
	 */
	void createRenderPass();

	/*
	 * @brief Creates the descriptor set layout that will allow the model to be 
	 * manipulated by the user.
	 */
	void createDescriptorSetLayout();

	/*
	 * @brief Creates the graphics pipeline.
	 */
	void createGraphicsPipeline();

	/*
	 * @brief Creates the framebuffers that will be used for rendering the
	 * swapchain images. We have to create a framebuffer for each image in the
	 * swapchain and use the one that corresponds to the retrieved image at draw
	 * time.
	 */
	void createFramebuffers();

	/*
	 * @brief Creates the command pool to manage the memory used to store buffers and
	 * command buffers that are allocated from them.
	 */
	void createCommandPool();

	/**
	 * @brief Copies the source buffer into the destination buffer.
	 */
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	/**
	 * @brief Creates a buffer.
	 */
	void createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer &buffer,
		VkDeviceMemory &bufferMemory
	);

	/**
	 * @brief Creates a vertex buffer to allow GPU to use vertex data specified in
	 * the program.
	 */
	void createVertexBuffer();

	/**
	 * @brief Creates an index buffer to avoid reuse of vertices.
	 */
	void createIndexBuffer();

	/**
	 * @brief Creates the uniform buffers and their associated memory.
	 */
	void createUniformBuffers();

	/**
	 * @brief Creates the descriptor pool for the descriptor sets.
	 */
	void createDescriptorPool();

	/**
	 * @brief Creates the descriptor sets associated with the descriptor pool.
	 */
	void createDescriptorSets();

	/*
	 * @brief Create and allocate command buffers to record drawing commands to.
	 */
	void createCommandBuffers();

	/*
	 * @brief Recreates the swapchain.
	 */
	void recreateSwapchain();

	/*
	 * @brief Create the semaphores used for synchronization of frame drawing.
	 */
	void createSyncObjects();

	/*
	 * @brief Sets up the necessary background checks to create a Vulkan instance.
	*/
	void initVulkan();

	/*
	 * @brief Updates the uniform buffer objects.
	 */
	void updateUniformBuffer(uint32_t currImg);

	/*
	 * @brief Draws a frame to the screen.
	 * - Aquires image from swapchain
	 * - Executes command buffer with image as attachment in framebuffer
	 * - Return image to swapchain for presentation
	 */
	void drawFrame();

	/*
	 * @brief Main game loop that keeps everything running.
	 */
	void main();

	/*
	 * @brief Cleans up all components relating to the swapchain.
	 */
	void cleanupSwapchain();

	/*
	 * @brief Cleans up Vulkan and GLFW.
	 */
	void cleanup();
};

#endif // GAME_HPP

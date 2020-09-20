#pragma once

#ifndef GAME_HPP
#define GAME_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "utils.hpp"
#include "constants.hpp"

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

	// pipeline layout
	VkPipelineLayout pipelineLayout;

	// final pipeline
	VkPipeline graphicsPipeline;

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
	 * @brief Sets up the necessary background checks to create a Vulkan instance.
	*/
	void initVulkan();

	/*
	 * @brief Main game loop that keeps everything running.
	 */
	void main();

	/*
	 * @brief Cleans up Vulkan and GLFW.
	 */
	void cleanup();
};

#endif // GAME_HPP

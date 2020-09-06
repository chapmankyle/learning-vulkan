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

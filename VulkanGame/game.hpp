#pragma once

#ifndef GAME_HPP
#define GAME_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

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
	vk::Instance instance;

	/*
	 * @brief Initializes a window.
	 */
	void initWindow();

	/*
	 * @brief Checks if the required extensions are available.
	 * @param required The names of the required extensions.
	 * @param available The currently available extensions.
	 * @returns `true` if the required extensions are present, `false` otherwise.
	 */
	bool hasRequiredExtensions(const std::vector<const char *> &required, const std::vector<vk::ExtensionProperties> &available);

	/*
	 * @brief Checks if all the requested layers are available.
	 * @returns `true` if validation layers are supported, `false` otherwise.
	 */
	bool hasValidationLayerSupport();

	/*
	 * @brief Creates a Vulkan instance and checks if the GLFW required extensions are
	 * available.
	 */
	void createInstance();

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

#endif

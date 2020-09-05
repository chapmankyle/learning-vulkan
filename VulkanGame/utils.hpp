#pragma once

#ifndef UTILS_HPP
#define UTILS_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <iostream>

#include "constants.hpp"

class Utils {
public:
	/*
	 * @returns The extensions that are available on the current machine.
	 */
	static std::vector<vk::ExtensionProperties> getAvailableExtensions();

	/*
	 * @returns The required extensions that Vulkan needs.
	 */
	static std::vector<const char*> getRequiredExtensions();

	/*
	 * @brief Checks if the required extensions are available.
	 * @param required The names of the required extensions.
	 * @param available The currently available extensions.
	 * @returns `true` if the required extensions are present, `false` otherwise.
	 */
	static bool hasRequiredExtensions(
		const std::vector<const char *> &required,
		const std::vector<vk::ExtensionProperties> &available
	);

	/*
	 * @brief Checks if all the requested layers are available.
	 * @returns `true` if validation layers are supported, `false` otherwise.
	 */
	static bool hasValidationLayerSupport();
};

#endif // UTILS_HPP


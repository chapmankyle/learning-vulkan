#pragma once

#ifndef COMMON_UTILS_HPP
#define COMMON_UTILS_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <vector>

#ifndef NDEBUG
#include <iostream>
#endif // !NDEBUG

namespace carbon {

#ifdef NDEBUG
	inline constexpr bool enableValidationLayers = false;
#else
	inline constexpr bool enableValidationLayers = true;
#endif // NDEBUG

	namespace utils {

		/*
		 * @returns The validation layers that are required by the engine.
		 */
		static inline const std::vector<const char *> getRequiredValidationLayers() {
			return std::vector<const char *>{ "VK_LAYER_KHRONOS_validation" };
		}

		/*
		 * @returns The extensions that are required by the engine.
		 */
		inline const std::vector<const char *> getRequiredExtensions();

		/*
		 * @brief Requests the validation layers that are supported on the current machine.
		 * @returns The validation layer properties for each supported validation layer.
		 */
		inline const std::vector<VkLayerProperties> requestSupportedValidationLayers();

		/*
		 * @brief Requests the extensions that are supported on the current machine.
		 * @returns The extension properties for each supported extension.
		 */
		inline const std::vector<VkExtensionProperties> requestSupportedExtensions();

		/*
		 * @brief Checks if the required layers are available.
		 * @returns `true` if all required layers are available, `false` otherwise.
		 */
		bool containsRequired(const std::vector<const char *> &required, const std::vector<VkLayerProperties> &available);

		/*
		 * @brief Checks if the required extensions are available.
		 * @returns `true` if all required extensions are available, `false` otherwise.
		 */
		bool containsRequired(const std::vector<const char *> &required, const std::vector<VkExtensionProperties> &available);

		/*
		 * @brief Checks if all the required layers are available.
		 * @returns `true` if the required validation layers are supported, `false` otherwise.
		 */
		bool hasValidationLayerSupport();

		/*
		 * @brief Checks if all the required extensions are available.
		 * @returns `true` if the required extensions are supported, `false` otherwise.
		 */
		bool hasExtensionSupport();

	} // namespace utils

} // namespace carbon

#endif // !COMMON_UTILS_HPP


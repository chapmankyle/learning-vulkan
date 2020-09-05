#pragma once

#ifndef UTILS_HPP
#define UTILS_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <optional>

#include "constants.hpp"

class Utils {
public:

	struct QueueFamilyIndices {
		// contains no value until something is assigned to it
		std::optional<uint32_t> graphicsFamily;

		/*
		 * @brief Check if the `graphicsFamily` has been assigned a value.
		 * @returns `true` if assigned a value, `false` otherwise.
		 */
		bool containsValue() {
			return graphicsFamily.has_value();
		}
	};

	/*
	 * @returns The extensions that are available on the current machine.
	 */
	static std::vector<VkExtensionProperties> getAvailableExtensions();

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
		const std::vector<VkExtensionProperties> &available
	);

	/*
	 * @brief Checks if all the requested layers are available.
	 * @returns `true` if validation layers are supported, `false` otherwise.
	 */
	static bool hasValidationLayerSupport();

	/*
	 * @brief Callback function to display debug messages when validation layers
	 * are turned on.
	 * @param messageSeverity Severity of the message (`VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT`,
	 * `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`, `VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT` or
	 * `VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT`)
	 * @param messageType Indicates what type of message has been generated (`VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT`,
	 * `VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT` or `VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT`)
	 * @param pCallbackData Refers to struct containing details of the message.
	 * @param pUserData Pointer to allow user to pass data into.
	 * @returns `true` if the Vulkan call that triggered validation layer message should be aborted
	 */
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData
	);

	/*
	 * @brief Populates the `VkDebugUtilsMessengerCreateInfo` struct with debug information
	 * that is relevant to display.
	 * @param createInfo The debug messenger to fill.
	 */
	static void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

	/*
	 * @brief Creates a debug utils messenger to use for debugging validation layers.
	 */
	static VkResult createDebugUtilsMessenger(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
		const VkAllocationCallbacks *pAllocator,
		VkDebugUtilsMessengerEXT *pDebugMessenger
	);

	/*
	 * @brief Destroys the debug utils messenger and frees memory.
	 */
	static void destroyDebugUtilsMessenger(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks *pAllocator
	);

	/*
	 * @brief Display the properties from the given device properties.
	 * @param deviceProps The device properties to display.
	 */
	static void showDeviceProperties(VkPhysicalDeviceProperties deviceProps);

	/*
	 * @brief Display the properties of the given device.
	 * @param device The device to display the properties of.
	 */
	static void showDeviceProperties(VkPhysicalDevice device);

	/*
	 * @brief Find the queue families that support graphics commands.
	 * @param device The device to check if contains the queue families.
	 * @returns The indices of the queue families found in the device.
	 */
	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	/*
	 * @brief Computes a score based on what properties and features the device has.
	 * @param device The physical device to compute a score for.
	 * @returns A score of how well suited the device is to the needs of the application.
	 */
	static int getDeviceScore(VkPhysicalDevice device);
};

#endif // UTILS_HPP


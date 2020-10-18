#pragma once

#ifndef UTILS_HPP
#define UTILS_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <array>
#include <algorithm> // for std::min() and std::max()
#include <cstdint>   // for UINT32_MAX
#include <fstream>
#include <iostream>
#include <optional>
#include <set>

#include "constants.hpp"

class Utils {
public:

	struct QueueFamilyIndices {
		// contains no value until something is assigned to it
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		/*
		 * @brief Check if the `graphicsFamily` has been assigned a value.
		 * @returns `true` if assigned a value, `false` otherwise.
		 */
		bool containsValue() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	/**
	 * @brief Structure to store vertex information.
	 */
	struct Vertex {
		glm::vec2 pos;
		glm::vec3 colour;

		/**
		 * @returns The rate at which to load data from memory throughout
		 * the vertices. Specifies number of bytes between data entries and
		 * whether to move to next data entry after each vertext or after
		 * each instance.
		 */
		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDesc{};

			bindingDesc.binding = 0; // index of binding in array of bindings
			bindingDesc.stride = sizeof(Vertex); // number of bytes from one entry to another
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // move to next data entry after each vertex

			return bindingDesc;
		}

		/**
		 * @returns How to extract a vertex attribute from a chunk of vertex data originating from
		 * a binding description.
		 */
		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDesc{};

			// position
			// - format:
			//   * float: VK_FORMAT_R32_SFLOAT
			//   * vec2:  VK_FORMAT_R32G32_SFLOAT
			//   * vec3:  VK_FORMAT_R32G32B32_SFLOAT
			//   * vec4:  VK_FORMAT_R32G32B32A32_SFLOAT
			attributeDesc[0].binding = 0;
			attributeDesc[0].location = 0;
			attributeDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDesc[0].offset = offsetof(Vertex, pos);

			// colour
			attributeDesc[1].binding = 0;
			attributeDesc[1].location = 1;
			attributeDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDesc[1].offset = offsetof(Vertex, colour);

			return attributeDesc;
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
	static void showDeviceProperties(const VkPhysicalDeviceProperties &deviceProps);

	/*
	 * @brief Display the properties of the given device.
	 * @param device The device to display the properties of.
	 */
	static void showDeviceProperties(const VkPhysicalDevice &device);

	/*
	 * @brief Find the queue families that support graphics commands.
	 * @param device The device to check if contains the queue families.
	 * @param surface The surface to query surface support for.
	 * @returns The indices of the queue families found in the device.
	 */
	static QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice &device, const VkSurfaceKHR &surface);

	/*
	 * @brief Checks if the device has extension support.
	 * @param device The device to check if extensions are supported.
	 * @returns `true` if the device has extension support, `false` otherwise.
	 */
	static bool hasDeviceExtensionSupport(const VkPhysicalDevice &device);

	/*
	 * @brief Computes a score based on what properties and features the device has.
	 * @param device The physical device to compute a score for.
	 * @param surface The surface to present rendered images to.
	 * @returns A score of how well suited the device is to the needs of the application.
	 */
	static int getDeviceScore(const VkPhysicalDevice &device, const VkSurfaceKHR &surface);

	/*
	 * @brief Queries the swapchain support of a device.
	 * @param device The device to query swapchain support from.
	 * @param surface The surface to use.
	 */
	static SwapChainSupportDetails querySwapChainSupport(
		const VkPhysicalDevice &device,
		const VkSurfaceKHR &surface
	);

	/*
	 * @brief Choose a suitable swap-surface format for best possible swap chain.
	 * @param availableFormats The currently available surface formats.
	 * @returns The format to use for the swap chain.
	 */
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

	/*
	 * @brief Choose a presentation mode for the swap chain. The most important setting for the
	 * swap chain. Available formats include:
	 * - VK_PRESENT_MODE_IMMEDIATE_KHR : images submitted are immediately displayed to screen (tearing)
	 * - VK_PRESENT_MODE_FIFO_KHR : display takes image from front of queue and program inserts rendered
	 * images at back of queue. If queue is full, program was to wait. Similar to vertical sync (vsync).
	 * - VK_PRESENT_MODE_FIFO_RELAXED : if queue is empty, instead of waiting for vertcial blank (moment
	 * the display is refreshed), the image is immediately displayed.
	 * - VK_PRESENT_MODE_MAILBOX_KHR : instead of blocking application when queue is full, the images
	 * that are already queued are replaced with newer ones. Can be used to implement triple buffering,
	 * as opposed to double buffering used by vsync.
	 * @param availablePresentModes The currently available presentation modes.
	 * @returns The presentation mode to use for the swap chain.
	 */
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

	/*
	 * @brief Choose the resolution of the swap chain images. Width and height of image is in
	 * `currentExtent` member, but if set to UINT32_MAX, then we set the resolution to one
	 * that best matches the window within the `minImageExtent` and `maxImageExtent` bounds.
	 * @param capabilities The range of possible resolutions.
	 * @returns The resolution that the swap chain images will be rendered in.
	 */
	static VkExtent2D chooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR &capabilities);

	/*
	 * @brief Reads the contents of a file and returns the bytes as characters.
	 * @param fileName The name of the file to read.
	 * @returns The contents of the file.
	 */
	static std::vector<char> readFile(const std::string &fileName);

	/*
	 * @brief Creates a shader module from the SPIR-V bytecode.
	 * @param code The bytecode to wrap.
	 * @return A shader module.
	 */
	static VkShaderModule createShaderModule(const VkDevice &device, const std::vector<char> &code);

	/**
	 * @brief Finds the type of memory that is compatible with our application requirements.
	 * @param device The physical device to get memory properties of.
	 * @param typeFilter The bit field of memory types that are suitable.
	 * @param props The flags for the memory properties.
	 */
	static uint32_t findMemoryType(const VkPhysicalDevice &device, uint32_t typeFilter, VkMemoryPropertyFlags props);

};

#endif // UTILS_HPP

#pragma once

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <vector>

namespace constants {

#ifdef NDEBUG
	inline constexpr bool enableValidationLayers = false;
#else
	inline constexpr bool enableValidationLayers = true;
#endif // NDEBUG

	inline constexpr int width{ 1920 };
	inline constexpr int height{ 1080 };

	inline constexpr bool fullscreen{ false };

	inline constexpr char title[]{ "Work In Progress: Game" };

	inline const std::vector<const char*> validationLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	inline const std::vector<const char *> deviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
}

#endif // CONSTANTS_H


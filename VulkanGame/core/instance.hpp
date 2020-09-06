#pragma once

#ifndef CORE_INSTANCE_HPP
#define CORE_INSTANCE_HPP

#include <vector>

#include "../common/utils.hpp"

namespace carbon {

	class Instance {

	private:

		/*
		 * @brief Handle to underlying Vulkan instance.
		 */
		VkInstance handle{ VK_NULL_HANDLE };

		/*
		 * @brief The enabled extensions on the current instance.
		 */
		std::vector<const char *> enabledExtensions;

	public:

		Instance(
			const char* appName,
			const std::vector<const char *> &requiredValidationLayers,
			const std::vector<const char *> &requiredExtensions
		);

		Instance(const VkInstance &inst);

		~Instance();

	};

} // namespace carbon

#endif // CORE_INSTANCE_HPP


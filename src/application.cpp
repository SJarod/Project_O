#include <exception>
#include <vector>
#include <iostream>

#include "utils/multithread/globalthreadpool.hpp"

#include "format.hpp"
#include "lowrenderer.hpp"

#include "application.hpp"

Application::Application()
{
	glfwInit();
	windowInit();

	// init GLFW for Vulkan
	if (!glfwVulkanSupported())
		throw std::exception("GLFW failed to find the Vulkan loader");

	// gather Vulkan extensions required by GLFW
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	// give the extensions to the low renderer (api instance)
	// create the Vulkan instance first
	rdr.api.initGraphicsAPI(requiredExtensions);

	// create a surface using the instance
	if (glfwCreateWindowSurface(rdr.api.instance, window, nullptr, &rdr.api.surface) != VK_SUCCESS)
		throw std::exception("Failed to create window surface");

	rdr.initRenderer();
}

Application::~Application()
{
	rdr.terminateRenderer();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::windowInit()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(Format::height, Format::width, "Vulkan window", nullptr, nullptr);
	glfwGetFramebufferSize(window, &Format::framebufferHeight, &Format::framebufferWidth);
}

#include "resourcesmanager.hpp"
#include "mesh.hpp"

void Application::loop()
{
	// TODO : store vbos in a scene

	ResourcesManager::load<Mesh>("triangle", "", new MeshRenderer(rdr));
	ResourcesManager::load<Mesh>("triangle", "", new MeshRenderer(rdr));
	ResourcesManager::load<Mesh>("triangle", "", new MeshRenderer(rdr));
	ResourcesManager::load<Mesh>("triangle", "", new MeshRenderer(rdr));
	ResourcesManager::load<Mesh>("triangle", "", new MeshRenderer(rdr));

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		Utils::GlobalThreadPool::pollMainQueue();

		//draw frame
		rdr.render();
	}
}
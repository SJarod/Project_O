#pragma once

#include <vector>

#include <GLFW/glfw3.h>

class Application
{
private:
	GLFWwindow* window;

	void windowInit();

public:
	Application();
	void destroy();

	void getWindowSize(int& width, int& height);

	void loop();
};
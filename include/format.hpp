#pragma once

#include "utils/singleton.hpp"

/**
 * Window's informational values.
 */
class Format : public Utils::Singleton<Format>
{
	SINGLETON(Format)

public:
	static int height;
	static int width;

	static int framebufferHeight;
	static int framebufferWidth;
};
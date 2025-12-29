#include "cursor.hpp"
#include "assert.hpp"
#include "window.hpp"
 
Cursor::Cursor(int systemCursorType): cursorHandle(glfwCreateStandardCursor(systemCursorType)) {
	// standard cursor creation shold NOT fail.
	Assert(cursorHandle != nullptr);
}

Cursor::Cursor(Cursor&& old)
{
	cursorHandle = old.cursorHandle;
	old.cursorHandle = nullptr;
}

Cursor& Cursor::operator=(Cursor&& other)
{

	cursorHandle = other.cursorHandle;
	other.cursorHandle = nullptr;
	return *this;
}

Cursor::Cursor() {
	cursorHandle = nullptr;
}

Cursor::~Cursor() noexcept {
	if (!Window::GLFW_INIT) return; // can't/needn't destroy glfw stuff if glfw was already terminated
	if (cursorHandle != nullptr)
		glfwDestroyCursor(cursorHandle);
}
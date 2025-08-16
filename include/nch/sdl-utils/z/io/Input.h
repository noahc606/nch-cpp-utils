#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <map>
#include <vector>

namespace nch { class Input {
public:
	enum InputTypeID {
		INVALID_0 = -1,
		KEY, KEYBOARD_MODIFIER, MOUSE, JOYBUTTON, JOYHATAXIS,
		INVALID_1,
	};

	enum MouseButton {
		LEFT_CLICK = 1,
		MIDDLE_CLICK = 2,
		RIGHT_CLICK = 3,
	};

	enum JoyHatDirection { NONE = 0, LEFT, UP, RIGHT, DOWN, };

	static void tick();
	static void allEvents(SDL_Event& e);
	static void inputEvents(SDL_Event& e);
	
    static SDL_Event getLastKnownSDLEvent();
    static int32_t getLastKnownSDLEventID();
	static int getMouseXAbs(), getMouseYAbs();
	static int getMouseX(), getMouseY();
	static int getMouseWheelDelta();
	static int keyDownTime(SDL_Keycode kc);
	static int mouseDownTime(int mouseButton);
	static int joystickButtonDownTime(int joyButton);
	static int joystickHatDirTime(int dir);
	static bool isKeyDown(SDL_Keycode kc);
	static bool isModKeyDown(SDL_Keymod km);
	static bool isMouseDown(int mouseButton);
	static bool isJoystickButtonDown(int joyButton);
	static bool isJoystickHatDir(int dir);

	static void setMouseViewport(const SDL_Rect& mvp);
	static void resetMouseViewport();

private:
	static void init();
	static void updInputState(InputTypeID inputType, int32_t sdlInputID, bool holdingDown);
	static int inputDownTime(InputTypeID inputType, int32_t sdlInputID);
};}
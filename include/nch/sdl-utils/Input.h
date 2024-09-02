#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <map>
#include <vector>

namespace nch { class Input {
public:
	enum InputTypeID {
		INVALID_0 = -1,
		KEY, KEYBOARD_MODIFIER, MOUSE, JOYBUTTON,
		INVALID_1,
	};

	enum MouseButton {
		LEFT = 1,
		MIDDLE = 2,
		RIGHT = 3,
	};

	static void tick();
	static void allEvents(SDL_Event& e);
	static void inputEvents(SDL_Event& e);
	static void mouse(SDL_Event& e);
	
    static SDL_Event getLastKnownSDLEvent();
	static int getMouseX();
	static int getMouseY();
	static int keyDownTime(SDL_Keycode kc);
	static int mouseDownTime(int mouseButton);
	static bool isKeyDown(SDL_Keycode kc);
	static bool isModKeyDown(SDL_Keymod km);
	static bool isMouseDown(int mouseButton);

private:
	static void init();
	static void updInputState(InputTypeID inputType, int32_t sdlInputID, bool holdingDown);
	static int inputDownTime(InputTypeID inputType, int32_t sdlInputID);

    static SDL_Event lastKnownEvent;
	static std::vector<std::map<int32_t, int>> inputStates;
	static std::map<int32_t, int> modKeyStates;
	static uint16_t currentModKeys;
	static SDL_Joystick* mainJoystick;

	static bool initialized;
};}
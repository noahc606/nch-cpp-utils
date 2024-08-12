#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <map>

class Input {
public:
	static void tick();
	static void events(SDL_Event& e);
	static void mouse(SDL_Event& e);
	
	static int getMouseX();
	static int getMouseY();
	static int keyDownTime(SDL_Keycode kc);
	static int keySDownTime();
	static bool isKeyDown(SDL_Keycode kc);
	static bool isModKeyDown(SDL_Keymod km);
	static bool isMouseDown(int mouseButton);

private:
	static void setKeyState(int32_t, int);
	static void setMouseState(int32_t, int);

	static std::map<int32_t, int> keyStates;
	static std::map<int32_t, int> modKeyStates;
	static std::map<int32_t, int> mouseStates;
	static uint16_t currentModKeys;

	static int holdingS;
};
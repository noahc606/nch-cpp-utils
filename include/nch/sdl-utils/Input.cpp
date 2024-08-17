#include "Input.h"
#include <nch/sdl-utils/debug/SDLEventDebugger.h>

SDL_Event Input::lastKnownEvent;
std::map<int32_t, int> Input::keyStates;
std::map<int32_t, int> Input::mouseStates;
uint16_t Input::currentModKeys = 0;

int Input::holdingS = 0;

void Input::tick()
{
	if(isKeyDown(SDLK_s)) {
		holdingS++;
	} else {
		holdingS = 0;
	}
	/*
	for(std::map<int32_t, int>::iterator itr = keyStates.begin(); itr!=keyStates.end(); itr++) {
		if(isKeyDown(itr->first)) {
			itr->second--;
		}
	}

	for(std::map<int32_t, int>::iterator itr = mouseStates.begin(); itr!=mouseStates.end(); itr++) {
		if(isMouseDown(itr->first)) {
			itr->second--;
		}
	}*/
}

void Input::anyEvents(SDL_Event& e)
{
	SDLEventDebugger sed;
	std::string eventDesc = sed.toString(e);
	if(eventDesc!="unknown") {
		lastKnownEvent = e;
		printf("%s\n", eventDesc.c_str());
	}
}

void Input::events(SDL_Event& e)
{
	switch(e.type) {
		case SDL_KEYDOWN: 			{ setKeyState(e.key.keysym.sym, -1); } break;
		case SDL_KEYUP: 			{ setKeyState(e.key.keysym.sym, 0); } break;
		case SDL_MOUSEBUTTONDOWN: 	{ setMouseState(e.button.button, -1); } break;
		case SDL_MOUSEBUTTONUP:		{ setMouseState(e.button.button, 0); } break;
	}

	currentModKeys = e.key.keysym.mod;
}

SDL_Event Input::getLastKnownSDLEvent() { return lastKnownEvent; }

int Input::getMouseX()
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return x;
}

int Input::getMouseY()
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return y;
}

bool Input::isKeyDown(SDL_Keycode kc) { return keyDownTime(kc)>0; }
int Input::keyDownTime(SDL_Keycode kc)
{
	auto ksItr = keyStates.find(kc);
	if(ksItr==keyStates.end()) {
		return 0;
	} else if (ksItr->second<0) {
		return -ksItr->second;
	}
	
	return 0;
}
int Input::keySDownTime()
{
	return holdingS;
}

bool Input::isModKeyDown(SDL_Keymod km)
{
	return (currentModKeys & km);
}

bool Input::isMouseDown(int mouseButton)
{
	auto msItr = mouseStates.find(mouseButton);
	if(msItr==mouseStates.end()) {
		return false;
	} else if (msItr->second<0) {
		return true;
	}
	return false;
}
void Input::setKeyState(int32_t key, int state)
{
	auto ksItr = keyStates.find(key);
	if( ksItr!=keyStates.end() ) {
		keyStates.erase(ksItr);
	}
	keyStates.insert( std::make_pair(key, state) );
}

void Input::setMouseState(int32_t mouseButton, int state)
{
	auto msItr = mouseStates.find(mouseButton);
	if( msItr!=mouseStates.end() ) {
		mouseStates.erase(msItr);
	}
	mouseStates.insert( std::make_pair(mouseButton, state) );
}
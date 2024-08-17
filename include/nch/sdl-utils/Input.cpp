#include "Input.h"
#include <nch/sdl-utils/debug/SDLEventDebugger.h>

SDL_Event NCH_Input::lastKnownEvent;
std::map<int32_t, int> NCH_Input::keyStates;
std::map<int32_t, int> NCH_Input::mouseStates;
uint16_t NCH_Input::currentModKeys = 0;

int NCH_Input::holdingS = 0;

void NCH_Input::tick()
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

void NCH_Input::anyEvents(SDL_Event& e)
{
	NCH_SDLEventDebugger sed;
	std::string eventDesc = sed.toString(e);
	if(eventDesc!="unknown") {
		lastKnownEvent = e;
	}
}

void NCH_Input::events(SDL_Event& e)
{
	switch(e.type) {
		case SDL_KEYDOWN: 			{ setKeyState(e.key.keysym.sym, -1); } break;
		case SDL_KEYUP: 			{ setKeyState(e.key.keysym.sym, 0); } break;
		case SDL_MOUSEBUTTONDOWN: 	{ setMouseState(e.button.button, -1); } break;
		case SDL_MOUSEBUTTONUP:		{ setMouseState(e.button.button, 0); } break;
	}

	currentModKeys = e.key.keysym.mod;
}

SDL_Event NCH_Input::getLastKnownSDLEvent() { return lastKnownEvent; }

int NCH_Input::getMouseX()
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return x;
}

int NCH_Input::getMouseY()
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return y;
}

bool NCH_Input::isKeyDown(SDL_Keycode kc) { return keyDownTime(kc)>0; }
int NCH_Input::keyDownTime(SDL_Keycode kc)
{
	auto ksItr = keyStates.find(kc);
	if(ksItr==keyStates.end()) {
		return 0;
	} else if (ksItr->second<0) {
		return -ksItr->second;
	}
	
	return 0;
}
int NCH_Input::keySDownTime()
{
	return holdingS;
}

bool NCH_Input::isModKeyDown(SDL_Keymod km)
{
	return (currentModKeys & km);
}

bool NCH_Input::isMouseDown(int mouseButton)
{
	auto msItr = mouseStates.find(mouseButton);
	if(msItr==mouseStates.end()) {
		return false;
	} else if (msItr->second<0) {
		return true;
	}
	return false;
}
void NCH_Input::setKeyState(int32_t key, int state)
{
	auto ksItr = keyStates.find(key);
	if( ksItr!=keyStates.end() ) {
		keyStates.erase(ksItr);
	}
	keyStates.insert( std::make_pair(key, state) );
}

void NCH_Input::setMouseState(int32_t mouseButton, int state)
{
	auto msItr = mouseStates.find(mouseButton);
	if( msItr!=mouseStates.end() ) {
		mouseStates.erase(msItr);
	}
	mouseStates.insert( std::make_pair(mouseButton, state) );
}
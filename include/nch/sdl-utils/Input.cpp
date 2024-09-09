#include "Input.h"
#include "nch/cpp-utils/io/Log.h"
#include "nch/sdl-utils/debug/SDLEventDebugger.h"


using namespace nch;

SDL_Event Input::lastKnownEvent;
int32_t Input::lastKnownEventID = -1;
std::vector<std::map<int32_t, int>> Input::inputStates;
std::map<int, int> Input::joyHatStates;
uint16_t Input::currentModKeys = 0;
SDL_Joystick* Input::mainJoystick = nullptr;

bool Input::initialized = false;

void Input::tick()
{
	if(initialized==false) {
		init();
		initialized = true;
	}

	//Update input hold times
	for(int i = 0; i<InputTypeID::INVALID_1; i++) {
		//Get ptr to inputStates[i], call it 'inmap'
		std::map<int32_t, int>* inmap = &inputStates[i];
		//Go thru 'inmap' and manage their itr->second value (represents # of ticks held down)
		for(auto itr = inmap->begin(); itr!=inmap->end(); itr++) {
			//printf("Found (%d, %d)\n", itr->first, itr->second);
			if(itr->second<0) {
				itr->second = 1;
			} else if(itr->second>0) {
				itr->second++;
			}
		}
	}
	
}
void Input::allEvents(SDL_Event& e)
{
	SDLEventDebugger sed;
	std::string eventDesc = sed.toString(e);
	if(eventDesc!="unknown") {
		lastKnownEvent = e;
	}
}
void Input::inputEvents(SDL_Event& e)
{
	switch(e.type) {
		case SDL_KEYDOWN: 			{ updInputState(InputTypeID::KEY, e.key.keysym.sym, true); } break;
		case SDL_KEYUP: 			{ updInputState(InputTypeID::KEY, e.key.keysym.sym, false); } break;
		case SDL_MOUSEBUTTONDOWN: 	{ updInputState(InputTypeID::MOUSE, e.button.button, true); } break;
		case SDL_MOUSEBUTTONUP:		{ updInputState(InputTypeID::MOUSE, e.button.button, false); } break;
		case SDL_JOYBUTTONDOWN:		{ updInputState(InputTypeID::JOYBUTTON, e.jbutton.button, true); } break;
		case SDL_JOYBUTTONUP:		{ updInputState(InputTypeID::JOYBUTTON, e.jbutton.button, false); } break;
		case SDL_JOYHATMOTION: {
			//Get joystick input info
			uint8_t joyHatIdx = e.jhat.hat;
			uint8_t joyHatVal = SDL_JoystickGetHat(mainJoystick, joyHatIdx);
			std::map<int, int>::iterator itr = joyHatStates.find(joyHatIdx);
			if(itr==joyHatStates.end()) {
				joyHatStates.insert(std::make_pair(joyHatIdx, joyHatVal));
			} else {
				itr->second = joyHatVal;
			}

			//Update joystick input state
			std::vector<int32_t> joyHatStateList = {
				SDL_HAT_CENTERED,
				SDL_HAT_LEFTDOWN, SDL_HAT_LEFT, SDL_HAT_LEFTUP,
				SDL_HAT_UP,
				SDL_HAT_RIGHTUP, SDL_HAT_RIGHT, SDL_HAT_RIGHTDOWN,
				SDL_HAT_DOWN,
			};
			for(int i = 0; i<9; i++) {
				if(joyHatVal==joyHatStateList[i]) {
					updInputState(InputTypeID::JOYHATAXIS, (int32_t)joyHatStateList[i], true);
				} else {
					updInputState(InputTypeID::JOYHATAXIS, (int32_t)joyHatStateList[i], false);
				}
			}
			
		} break;
	}

	currentModKeys = e.key.keysym.mod;
}

SDL_Event Input::getLastKnownSDLEvent() { return lastKnownEvent; }
int32_t Input::getLastKnownSDLEventID() { return lastKnownEventID; }
int Input::getMouseX() { int x; SDL_GetMouseState(&x, NULL); return x; }
int Input::getMouseY() { int y; SDL_GetMouseState(NULL, &y); return y; }

int Input::keyDownTime(SDL_Keycode kc) { return inputDownTime(InputTypeID::KEY, kc); }
int Input::mouseDownTime(int mouseButton) { return inputDownTime(InputTypeID::MOUSE, mouseButton); }
int Input::joystickButtonDownTime(int joyButton) { return inputDownTime(InputTypeID::JOYBUTTON, joyButton); }
int Input::joystickHatDirTime(int dir)
{
	int temp = 0;
	switch(dir) {
		case LEFT: {
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_LEFTUP); 		if(temp>0) return temp;
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_LEFT); 		if(temp>0) return temp;
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_LEFTDOWN); 	if(temp>0) return temp;
		} break;
		case UP: {
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_LEFTUP); 		if(temp>0) return temp;
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_UP); 			if(temp>0) return temp;
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_RIGHTUP); 	if(temp>0) return temp;
		} break;
		case RIGHT: {
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_RIGHTUP); 	if(temp>0) return temp;
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_RIGHT); 		if(temp>0) return temp;
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_RIGHTDOWN); 	if(temp>0) return temp;
		} break;
		case DOWN: {
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_LEFTDOWN); 	if(temp>0) return temp;
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_DOWN); 		if(temp>0) return temp;
			temp = inputDownTime(InputTypeID::JOYHATAXIS, SDL_HAT_RIGHTDOWN); 	if(temp>0) return temp;
		} break;
	}
	
	return 0;
}
bool Input::isKeyDown(SDL_Keycode kc) { return keyDownTime(kc)>0; }
bool Input::isModKeyDown(SDL_Keymod km) { return (currentModKeys & km); }
bool Input::isMouseDown(int mouseButton) { return mouseDownTime(mouseButton)>0; }
bool Input::isJoystickButtonDown(int joyButton) { return joystickButtonDownTime(joyButton)>0; }
bool Input::isJoystickHatDir(int dir) { return joystickHatDirTime(dir)>0; }

void Input::init()
{
	if(initialized) return;
	
	//Setup inputStates
	for(int i = 0; i<INVALID_1; i++) {
		inputStates.push_back(std::map<int32_t, int>());
	}

	//Open joystick(s)
	if(SDL_NumJoysticks()>0) {
		mainJoystick = SDL_JoystickOpen(0);
		nch::Log::log("Found joystick \"%s\"...\n", SDL_JoystickName(mainJoystick));

		int numHats = SDL_JoystickNumHats(mainJoystick);
		for(int i = 0; i<numHats; i++) {
			joyHatStates.insert(std::make_pair(i, 0));
		}
	}
}

void Input::updInputState(InputTypeID inputType, int32_t sdlInputID, bool holdingDown)
{
	lastKnownEventID = sdlInputID;

	//Try to get current key ID
	int iid = inputType;
	int32_t sid = sdlInputID;
	auto ksItr = inputStates[iid].find(sid);

	//If we are not holding down, try to get rid of the ID, and return.
	if(!holdingDown) {
		if(ksItr!=inputStates[iid].end())
			inputStates[iid].erase(ksItr);
		return;
	}

	//If we are holding down, continue...

	//If the ID doesn't doesn't exist, add new state value (-1)
	if( ksItr==inputStates[iid].end() ) {
		inputStates[iid].insert( std::make_pair(sid, -1) );
	}
}

int Input::inputDownTime(InputTypeID inputType, int32_t sdlInputID)
{
	if(!initialized) {
		nch::Log::warnv(__PRETTY_FUNCTION__, "returning 0", "Input not initialized (it seems a MainLoopDriver() was not created)");
		return 0;
	}

	auto ksItr = inputStates[inputType].find(sdlInputID);
	if(ksItr==inputStates[inputType].end()) {
		return 0;
	} else if (ksItr->second>0) {
		return ksItr->second;
	}
	
	return 0;
}
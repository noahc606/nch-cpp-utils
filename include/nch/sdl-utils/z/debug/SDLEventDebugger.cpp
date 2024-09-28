#include "SDLEventDebugger.h"

using namespace nch;

std::string SDLEventDebugger::toString(SDL_Event e)
{
    //Descriptions taken from SDL2/SDL_events.h
    std::string type = "unknown";
    switch(e.type) {
        //Keyboard
        case SDL_KEYDOWN:                   type = "Key pressed (SDL_KEYDOWN)"; break;
        case SDL_KEYUP:                     type = "Key released (SDL_KEYUP)"; break;
        case SDL_TEXTEDITING:               type = "Keyboard text editing - composition (SDL_TEXTEDITING)"; break;
        case SDL_TEXTINPUT:                 type = "Keyboard text input (SDL_TEXTINPUT)"; break;
        case SDL_KEYMAPCHANGED:             type = "Keymap change due to language/layout change (SDL_KEYMAPCHANGED)"; break;

        //Mouse
        case SDL_MOUSEMOTION:               type = "Mouse moved (SDL_MOUSEMOTION)"; break;
        case SDL_MOUSEBUTTONDOWN:           type = "Mouse button pressed (SDL_MOUSEBUTTONDOWN)"; break;
        case SDL_MOUSEBUTTONUP:             type = "Mouse button released (SDL_MOUSEBUTTONUP)"; break;
        case SDL_MOUSEWHEEL:                type = "Mouse wheel motion (SDL_MOUSEWHEEL)"; break;

        //Joystick/controller
        case SDL_JOYAXISMOTION:             type = "Joystick axis motion (SDL_JOYAXISMOTION)"; break;
        case SDL_JOYBALLMOTION:             type = "Joystick trackball motion (SDL_JOYBALLMOTION)"; break;
        case SDL_JOYHATMOTION:              type = "Joystick hat position change (SDL_JOYHATMOTION)"; break;
        case SDL_JOYBUTTONDOWN:             type = "Joystick button pressed (SDL_JOYBUTTONDOWN)"; break;
        case SDL_JOYBUTTONUP:               type = "Joystick button released (SDL_JOYBUTTONUP)"; break;
        case SDL_JOYDEVICEADDED:            type = "A new joystick has been inserted into the system (SDL_JOYDEVICEADDED)"; break;
        case SDL_JOYDEVICEREMOVED:          type = "An opened joystick has been removed (SDL_JOYDEVICEREMOVED)"; break;
        
        //case SDL_JOYBATTERYUPDATED:         type = "Joystick battery level change (SDL_JOYBATTERYUPDATED)"; break;
        case SDL_CONTROLLERAXISMOTION:      type = "Game controller axis motion (SDL_CONTROLLERAXISMOTION)"; break;
        case SDL_CONTROLLERBUTTONDOWN:      type = "Game controller button pressed (SDL_CONTROLLERBUTTONDOWN)"; break;
        case SDL_CONTROLLERBUTTONUP:        type = "Game controller button released (SDL_CONTROLLERBUTTONUP)"; break;
        case SDL_CONTROLLERDEVICEADDED:     type = "A new Game controller has been inserted into the system (SDL_CONTROLLERDEVICEADDED)"; break;
        case SDL_CONTROLLERDEVICEREMOVED:   type = "An opened Game controller has been removed (SDL_CONTROLLERDEVICEREMOVED)"; break;
        case SDL_CONTROLLERDEVICEREMAPPED:  type = "The controller mapping was updated (SDL_CONTROLLERDEVICEREMAPPED)"; break;
        //case SDL_CONTROLLERTOUCHPADDOWN:    type = "Game controller touchpad was touched (SDL_CONTROLLERTOUCHPADDOWN)"; break;
        //case SDL_CONTROLLERTOUCHPADMOTION:  type = "Game controller touchpad finger was moved (SDL_CONTROLLERTOUCHPADMOTION)"; break;
        //case SDL_CONTROLLERTOUCHPADUP:      type = "Game controller touchpad finger was lifted (SDL_CONTROLLERTOUCHPADUP)"; break;
        //case SDL_CONTROLLERSENSORUPDATE:    type = "Game controller sensor was updated (SDL_CONTROLLERSENSORUPDATE)"; break;
    
        //Touchscreen
        case SDL_FINGERDOWN:                type = "Pressing down on touchscreen (SDL_FINGERDOWN)"; break;
        case SDL_FINGERUP:                  type = "Releasing from touchscreen (SDL_FINGERUP)"; break;
        case SDL_FINGERMOTION:              type = "Swiping across touchscreen (SDL_FINGERMOTION)"; break;

    }

    return "SDL_Event: "+type;
}
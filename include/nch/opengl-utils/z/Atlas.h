#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <map>
#include <nch/sdl-utils/rect.h>
#include <string>
#include <vector>
#include "nch/opengl-utils/shader.h"


namespace nch { class Atlas {
public:
    struct ImageInfo {
        std::string name;
        SDL_Surface* surface;
        int w, h;
    };

    
    Atlas(Atlas* base, std::string path, GLuint slot);
    Atlas(SDL_Surface* surf, GLuint slot);
    Atlas(std::string path, GLuint slot);
    Atlas(Atlas&& obj) {
        unit = obj.unit;
        id = obj.id;
        type = obj.type;
        map = obj.map;
        mapSize = obj.mapSize;

        //ID 0 prevents deletion of original. Ownership of texture moves to the new obj.
        obj.id = 0;
    }
    ~Atlas();

    GLuint getID();
    const std::string& getType();
    nch::FRect getSrc(const std::string& imgID);
    std::map<std::string, nch::Rect> getMap();
    int getMapSize();

    static void bind(GLenum unit, GLuint id);
    void bind();
    static void unbind();
    void texUnit(nch::Shader* shader, const char* uniform, GLuint unit);
private:
    static std::map<std::string, SDL_Surface*> collectImagesFromDir(std::string dirPath);
    void buildVariantFromDir(Atlas* base, std::string dirPath, GLuint slot);
    void buildFromDir(std::string dirPath, GLuint slot);
    void buildFromSDL_Surface(SDL_Surface* surf, GLuint slot);
    void buildFromImg(std::string imgPath, GLuint slot);
    /// @brief Generate a single OpenGL texture (created within 'textures') from the provided SDL_Surface*.
    static void buildGL_TextureFromSDL_Surface(SDL_Surface* surf);
    static std::map<std::string, nch::Rect> buildSquareAtlas(const std::map<std::string, SDL_Surface*>& collection, int& outSize);
    static bool tryPackMaxRects(int size, const std::vector<ImageInfo>& images, std::map<std::string, nch::Rect>& atlas);

    GLenum unit;
    GLuint id = 0;
    std::string type;
    std::map<std::string, nch::Rect> map;
    int mapSize = 0;
}; }

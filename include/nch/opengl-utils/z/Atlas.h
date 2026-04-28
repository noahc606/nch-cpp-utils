#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <map>
#include <nch/sdl-utils/rect.h>
#include <string>
#include <vector>
#include "nch/opengl-utils/shader.h"
#include "nch/opengl-utils/z/atlas/AtlasImage.h"

namespace nch { class Atlas {
public:
    struct BuildInfo {
        enum class Source { MultiplePaths, Path, Surface };
        Source source = Source::Path;
        Atlas* base = nullptr;
        std::vector<std::string> multPaths;
        std::vector<std::string> multPrefixes;
        std::string singlePath = "";
        std::string singlePrefix = "";
        SDL_Surface* surf = nullptr;
        GLuint slot = 0;
    };

    Atlas(Atlas* base, const std::vector<std::string>& paths, const std::vector<std::string>& prefixes, GLuint slot);
    Atlas(Atlas* base, std::pair<std::string, std::string> pathAndPrefix, GLuint slot);
    Atlas(Atlas* base, std::string path, GLuint slot);
    Atlas(const std::vector<std::string>& paths, const std::vector<std::string>& prefixes, GLuint slot);
    Atlas(std::pair<std::string, std::string> pathAndPrefix, GLuint slot);
    Atlas(std::string path, GLuint slot);
    Atlas(SDL_Surface* surf, GLuint slot);
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
    nch::FRect getSrc(const std::string& imgID) const;
    std::map<std::string, nch::Rect> getMap();
    int getMapSize();

    void reload();
    static void bind(GLenum unit, GLuint id);
    void bind();
    static void unbind();
    void texUnit(nch::Shader* shader, const char* uniform, GLuint unit);
    void saveDump(const std::string& imgPath);
private:
    void destroy();
    void build();

    void buildVariantFromDirs(Atlas* base, const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes, GLuint slot);
    void buildFromDirs(const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes, GLuint slot);
    void buildFromSDL_Surface(SDL_Surface* surf, GLuint slot);
    void buildFromImg(std::string imgPath, GLuint slot);

    BuildInfo buildInfo;
    GLenum unit = 0;
    GLuint id = 0;
    std::string type = "";
    std::map<std::string, nch::Rect> map;
    int mapSize = 0;
    bool built = false;
}; }

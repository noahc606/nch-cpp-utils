#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <map>
#include <nch/sdl-utils/rect.h>
#include <string>
#include <vector>

namespace nch { class AtlasImage {
public:
    std::string name;
    SDL_Surface* surface = nullptr;
    int w = 0, h = 0;

    static constexpr int PAD = 8;
    static constexpr float PAD_F = PAD;

    static std::map<std::string, SDL_Surface*> collectFromPaths(const std::vector<std::vector<std::string>>& objCollections, const std::vector<std::string>& collectionPrefixes, bool jsonFiles);
    static std::map<std::string, SDL_Surface*> collectFromDirs(const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes = {}, bool jsonFiles = false);
    static std::map<std::string, SDL_Surface*> collectFromDir(const std::string& dirPath, const std::string& prefix = "", bool jsonFiles = false);
    static void buildGLTexture(SDL_Surface* surf);
    static std::map<std::string, nch::Rect> buildSquareAtlas(const std::map<std::string, SDL_Surface*>& collection, int& outSize);
    static void blitWithPadding(SDL_Surface* src, SDL_Surface* dst, int dstX, int dstY);
private:
    static bool tryPackMaxRects(int size, const std::vector<AtlasImage>& images, std::map<std::string, nch::Rect>& atlas);
    static SDL_Surface* buildSurfaceFromJSON(const std::string& jsonPath);
}; }

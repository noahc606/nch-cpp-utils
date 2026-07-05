#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <map>
#include <nch/sdl-utils/rect.h>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace nch { class AtlasImage {
public:
    std::string name;
    SDL_Surface* surface = nullptr;
    int w = 0, h = 0;

    static constexpr int PAD = 8;
    static constexpr float PAD_F = PAD;

    //Placement of one image within a paged atlas: pixel rect + page (texture array layer) index.
    struct Entry {
        nch::Rect r;
        int page = 0;
    };

    //A time-animated image: frame surfaces (each the base image's dimensions) + playback timing.
    struct AnimSpec {
        int fps = 12;
        bool loop = true;
        std::vector<SDL_Surface*> frames;
    };

    /**
     * @brief Load images from the given file paths, keyed by "<prefix>/<subdirs>/<filename-no-ext>".
     * @param collectionRoots Root dir of each collection; when provided, subdirectories between the
     *        root and the file are kept in the key (omit for bare "<prefix>/<filename-no-ext>" keys).
     * @param outAnims When non-null, JSON entries carrying an "animation" block contribute an AnimSpec
     *        keyed identically to the base image; caller owns and must free the frame surfaces.
     */
    static std::map<std::string, SDL_Surface*> collectFromPaths(const std::vector<std::vector<std::string>>& objCollections, const std::vector<std::string>& collectionPrefixes, bool jsonFiles, const std::vector<std::string>& collectionRoots = {}, std::map<std::string, AnimSpec>* outAnims = nullptr);
    static std::map<std::string, SDL_Surface*> collectFromDirs(const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes = {}, bool jsonFiles = false, std::map<std::string, AnimSpec>* outAnims = nullptr);
    static std::map<std::string, SDL_Surface*> collectFromDir(const std::string& dirPath, const std::string& prefix = "", bool jsonFiles = false);
    static void buildGLTexture(SDL_Surface* surf);
    /**
     * @brief Upload pages as the layers of the currently bound GL_TEXTURE_2D_ARRAY.
     * @param pages One surface per layer; all must share the same dimensions.
     */
    static void buildGLTextureArray(const std::vector<SDL_Surface*>& pages);
    static std::map<std::string, nch::Rect> buildSquareAtlas(const std::map<std::string, SDL_Surface*>& collection, int& outSize);
    /**
     * @brief Pack images into equal-sized square pages no larger than maxPageSize.
     *        A collection that fits within one page shrinks to fit (same result as buildSquareAtlas);
     *        otherwise every page is maxPageSize wide and overflow spills largest-first onto new pages.
     * @return Placements keyed by image name.
     */
    static std::map<std::string, Entry> buildPagedAtlas(const std::map<std::string, SDL_Surface*>& collection, int maxPageSize, int& outSize, int& outPageCount);
    static void blitWithPadding(SDL_Surface* src, SDL_Surface* dst, int dstX, int dstY);
private:
    static bool tryPackMaxRects(int size, const std::vector<AtlasImage>& images, std::map<std::string, nch::Rect>& atlas);
    //Collection -> AtlasImage list sorted largest-first (better packing).
    static std::vector<AtlasImage> sortedBySize(const std::map<std::string, SDL_Surface*>& collection);
    static SDL_Surface* buildSurfaceFromJSON(const std::string& jsonPath);
    //Compose one image from an "applied_elements" array (layered img + colormod); nullptr on failure.
    static SDL_Surface* compositeFromElements(const nlohmann::json& appliedElements, const std::string& jsonDir);
    //Parse an "animation" block if present; true (and fills out) only when >=1 frame was built.
    static bool parseAnimationFromJSON(const std::string& jsonPath, AnimSpec& out);
}; }

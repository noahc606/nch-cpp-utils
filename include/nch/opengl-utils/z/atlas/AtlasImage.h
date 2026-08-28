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
    //Ceiling on the cartesian product of one JSON's wildcards, so a typo can't try to build millions.
    static constexpr int MAX_WILDCARD_EXPANSIONS = 4096;

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
     *
     * A JSON declaring "wildcards" contributes one image per token combination instead of one per
     * file, each keyed by its own "names" template in place of the filename (see expandWildcards).
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
    /**
     * @brief Expand a doc's "wildcards" block into one fully resolved document per token combination.
     *
     * "wildcards" maps an axis name to either a plain token list ("size": ["1","2","3"]) or a token ->
     * properties object ("metal": {"copper": {"color": [255,127,0]}}); the expansion is the cartesian
     * product of every axis. Within the doc, "{axis}" resolves to the current token and "{axis.prop}"
     * to that token's property — "{size}" and "{metal.color}" for the two above. A string that is
     * *exactly* one placeholder adopts the substitution's own JSON type, so "{metal.color}" yields the
     * array rather than its text, while "ov/ore_{size}.png" interpolates as ordinary text. "names" is
     * the same-syntax template naming each expansion, standing in for the filename the key would
     * otherwise take.
     * @param outNames Image name per combination.
     * @param outDocs Array of resolved copies of 'doc', parallel to outNames.
     * @return False when 'doc' declares no wildcards (or declares them unusably), outputs untouched.
     */
    static bool expandWildcards(const nlohmann::json& doc, const std::string& srcPath, std::vector<std::string>& outNames, nlohmann::json& outDocs);
    //Resolve every "{placeholder}" in 'node' and its descendants against 'subs'.
    static void substitutePlaceholders(nlohmann::json& node, const nlohmann::json& subs, const std::string& srcPath);
    //Textual "{placeholder}" expansion within one string; a non-string substitution is dumped compactly.
    static std::string substitutedString(const std::string& s, const nlohmann::json& subs, const std::string& srcPath);
    //Compose one image from a parsed doc's "applied_elements"; nullptr (having logged 'srcPath') on failure.
    static SDL_Surface* buildSurfaceFromDoc(const nlohmann::json& doc, const std::string& jsonDir, const std::string& srcPath);
    //False for a null or oversized surface, logging the latter; the caller still owns and frees it.
    static bool isSurfaceAtlasable(SDL_Surface* surf, const std::string& srcPath);
    //Compose one image from an "applied_elements" array (layered img + colormod); nullptr (having
    //logged 'srcPath', which only names the JSON - 'jsonDir' is what "img" paths resolve against) on failure.
    static SDL_Surface* compositeFromElements(const nlohmann::json& appliedElems, const std::string& jsonDir, const std::string& srcPath);
    /**
     * @brief Read a "colormod" value: an array of 3 or 4 ints, or a 6/8-digit hex string ("#00aaff", "ffffffaa").
     * @return False (leaving the outputs untouched) when the value is neither form.
     */
    static bool parseColormod(const nlohmann::json& cm, Uint8& outR, Uint8& outG, Uint8& outB, Uint8& outA);
    //Return a mirrored copy of 'src' (horizontally and/or vertically), freeing 'src'; returns 'src' unchanged when neither flag is set.
    static SDL_Surface* mirrorSurface(SDL_Surface* src, bool mirrorH, bool mirrorV);
    /**
     * @brief Return a copy of 'src' rotated clockwise, freeing 'src'.
     * @param numTurnsCW Number of clockwise quarter turns (negative = counter-clockwise), taken mod 4.
     * @return The rotated copy, or 'src' unchanged when the rotation is a no-op. Odd turn counts swap width and height.
     */
    static SDL_Surface* rotateSurface(SDL_Surface* src, int numTurnsCW);
    //Parse a doc's "animation" block if present; true (and fills out) only when >=1 frame was built.
    static bool parseAnimation(const nlohmann::json& doc, const std::string& jsonDir, const std::string& srcPath, AnimSpec& out);
}; }

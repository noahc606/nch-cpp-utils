#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <cstdint>
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

    //Placement of one image: pixel rect within its page + page (texture array layer) index.
    typedef AtlasImage::Entry Entry;

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
        pageCount = obj.pageCount;
        anims = std::move(obj.anims);
        lastAnimMS = obj.lastAnimMS;

        //ID 0 prevents deletion of original. Ownership of texture moves to the new obj.
        obj.id = 0;
    }
    ~Atlas();

    GLuint getID();
    const std::string& getType();
    /**
     * @brief UV rect for an image, with its page (texture array layer) index folded into u:
     *        u ∈ [page, page+1), v ∈ [0, 1). Single-page atlases return u ∈ [0, 1) as before.
     */
    nch::FRect getSrc(const std::string& imgID) const;
    std::map<std::string, Entry> getMap();
    int getMapSize();
    int getPageCount();

    void reload();
    /**
     * @brief Advance any time-animated entries and re-upload only the frames that changed.
     *        Binds this atlas' GL_TEXTURE_2D_ARRAY, so call within a live GL context (not inside
     *        a GLSDL SDL save/restore section). Cheap when idle: throttled and self-guarded per entry.
     */
    void animFrames();
    static void bind(GLenum unit, GLuint id);
    void bind();
    static void unbind();
    void texUnit(nch::Shader* shader, const char* uniform, GLuint unit);
    /**
     * @brief Save the atlas pixels as PNG(s). Multi-page atlases write one file per page,
     *        with "_p<N>" inserted before the extension.
     */
    void saveDump(const std::string& imgPath);

    //Force smaller pages than the GL limit (e.g. 512 to exercise multi-page paths on any GPU). <=0: use GL limit.
    static void setMaxPageSizeOverride(int px);
    //Max rate at which animFrames() re-uploads changed frames, across all atlases. Clamped to >=1.
    static void setAnimFPS(int fps);
private:
    void destroy();
    void build();
    //min(GL_MAX_TEXTURE_SIZE, override if set)
    static int resolveMaxPageSize();

    void buildVariantFromDirs(Atlas* base, const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes, GLuint slot);
    void buildFromDirs(const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes, GLuint slot);
    void buildFromSDL_Surface(SDL_Surface* surf, GLuint slot);
    void buildFromImg(std::string imgPath, GLuint slot);
    //Turn collected AnimSpecs into padded, GL-ready frame byte buffers bound to their packed entries;
    //frees each spec's frame surfaces.
    void buildAnims(std::map<std::string, AtlasImage::AnimSpec>& specs);

    static int maxPageSizeOverride;
    static int animFPS;

    //One time-animated entry: its packed cell (rect + page), timing, and per-frame ABGR8888 byte buffers.
    struct Anim {
        nch::Rect r;
        int page = 0;
        int fps = 12;
        bool loop = true;
        std::vector<std::vector<uint8_t>> frameBytes;
        int lastFrame = -1;
    };

    BuildInfo buildInfo;
    GLenum unit = 0;
    GLuint id = 0;
    std::string type = "";
    std::map<std::string, Entry> map;
    int mapSize = 0;
    int pageCount = 1;
    std::vector<Anim> anims;
    uint64_t lastAnimMS = 0;
    bool built = false;
}; }

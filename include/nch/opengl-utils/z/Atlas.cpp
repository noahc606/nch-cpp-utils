#include "Atlas.h"
#ifdef NCH_GLSDL_OPENGL_BACKEND
#include <GLSDL/GLSDL.h>
#endif
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <nch/cpp-utils/fs-utils.h>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/string-utils.h>
#include <nch/cpp-utils/timer.h>
#include <stdexcept>
using namespace nch;

Atlas::Atlas(Atlas* base, const std::vector<std::string>& paths, const std::vector<std::string>& prefixes, GLuint slot)
{
	buildInfo.source = BuildInfo::Source::MultiplePaths;
	buildInfo.base = base;
	buildInfo.multPaths = paths;
	buildInfo.multPrefixes = prefixes;
	buildInfo.slot = slot;
	build();
}
Atlas::Atlas(Atlas* base, std::pair<std::string, std::string> pathAndPrefix, GLuint slot)
{
	buildInfo.source = BuildInfo::Source::Path;
	buildInfo.base = base;
	buildInfo.singlePath = pathAndPrefix.first;
	buildInfo.singlePrefix = pathAndPrefix.second;
	buildInfo.slot = slot;
	build();
}
Atlas::Atlas(Atlas* base, std::string path, GLuint slot):
Atlas(base, {path, ""}, slot) {}

Atlas::Atlas(const std::vector<std::string>& paths, const std::vector<std::string>& prefixes, GLuint slot):
Atlas(nullptr, paths, prefixes, slot) {}
Atlas::Atlas(std::pair<std::string, std::string> pathAndPrefix, GLuint slot):
Atlas(nullptr, pathAndPrefix, slot) {}
Atlas::Atlas(std::string path, GLuint slot):
Atlas(nullptr, path, slot) {}
Atlas::Atlas(SDL_Surface* surf, GLuint slot)
{
	buildInfo.source = BuildInfo::Source::Surface;
	buildInfo.surf = surf;
	buildInfo.slot = slot;
	build();
}

Atlas::~Atlas() {
	destroy();
}

GLuint Atlas::getID() {
	return id;
}
const std::string& Atlas::getType() {
    return type;
}
FRect Atlas::getSrc(const std::string& imgID) const {
	auto itr = map.find(imgID);
	if(itr!=map.end()) {
		FRect fr = FRect(itr->second.r.x+AtlasImage::PAD_F, itr->second.r.y+AtlasImage::PAD_F, itr->second.r.w-2*AtlasImage::PAD_F, itr->second.r.h-2*AtlasImage::PAD_F);
		fr.scale(1.0f/mapSize);
		return fr;
	}

	throw std::runtime_error(nch::cat("Key \"", imgID, "\" does not exist within this atlas"));
}
std::map<std::string, nch::Rect> Atlas::getMap() {
	return map;
}
int Atlas::getMapSize() {
	return mapSize;
}

void Atlas::bind(GLenum unit, GLuint id)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D, id);
}
void Atlas::bind() {
	bind(unit, id);
}
void Atlas::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
void Atlas::texUnit(Shader* shader, const char* uniform, GLuint unit)
{
    GLuint texUni = glGetUniformLocation(shader->getID(), uniform);
    shader->useProgram();
    glUniform1i(texUni, unit);
}
void Atlas::saveDump(const std::string& imgPath) {
#ifdef NCH_GLSDL_OPENGL_BACKEND
	SDL_Surface* surf = GLSDL_Renderer::surfaceReadPixels(id, mapSize, mapSize, 4);
	IMG_SavePNG(surf, imgPath.c_str());
	SDL_FreeSurface(surf);
#else
	Log::warnv(__PRETTY_FUNCTION__, "doing nothing", "Function requires GLSDL and NCH_GLSDL_OPENGL_BACKEND to be defined");
#endif
}

void Atlas::reload() {
	destroy();
	build();
}
void Atlas::destroy() {
	unit = 0;
	if(id!=0) glDeleteTextures(1, &id);
	type = "";
	map.clear();
	mapSize = 0;
	built = false;
}
void Atlas::build() {
	switch(buildInfo.slot) {
		case 0: { type = "diffuse"; } break;
		case 1: { type = "specular"; } break;
	}
	unit = buildInfo.slot;

	if(buildInfo.source==BuildInfo::Source::Surface) {
		buildFromSDL_Surface(buildInfo.surf, buildInfo.slot);
		built = true;
		return;
	}
	if(buildInfo.source==BuildInfo::Source::Path) {
		//Assume building from files within dir
		if(FsUtils::dirExists(buildInfo.singlePath)) {
			if(buildInfo.base==nullptr) buildFromDirs({buildInfo.singlePath}, { buildInfo.singlePrefix }, buildInfo.slot);
			if(buildInfo.base!=nullptr) buildVariantFromDirs(buildInfo.base, {buildInfo.singlePath}, { buildInfo.singlePrefix }, buildInfo.slot);
			built = true;
			return;
		}
		//Assume building from single image file
		if(FsUtils::fileExists(buildInfo.singlePath)) {
			buildFromImg(buildInfo.singlePath, buildInfo.slot);
			built = true;
			return;
		}
		throw std::runtime_error(nch::cat("Could not resolve object \"", buildInfo.singlePath, "\" as a file or dir"));
	}

	if(buildInfo.source==BuildInfo::Source::MultiplePaths) {
		//Matching number of paths and prefixes?
		if(buildInfo.multPaths.size()!=buildInfo.multPrefixes.size())
			throw std::runtime_error("Expected number of paths to == number of prefixes");
		//Do all dirs exist?
		for(size_t i = 0; i<buildInfo.multPaths.size(); i++) {
			if(!FsUtils::dirExists(buildInfo.multPaths[i])) {
				throw std::runtime_error(nch::cat("Directory \"", buildInfo.multPaths[i], "\" doesn't exist"));
			}
		}
		
		//Building from multiple dir paths
		if(buildInfo.base==nullptr) { buildFromDirs(buildInfo.multPaths, buildInfo.multPrefixes, buildInfo.slot); }
		if(buildInfo.base!=nullptr) { buildVariantFromDirs(buildInfo.base, buildInfo.multPaths, buildInfo.multPrefixes, buildInfo.slot); }
		built = true;
		return;
	}
}
void Atlas::buildVariantFromDirs(Atlas* base, const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes, GLuint slot)
{
	//Collect images to be placed in the atlas...
	auto collection = AtlasImage::collectFromDirs(dirPaths, prefixes, true);

	//Build the final atlas surface...
	SDL_Surface* finalAtlasSurf;
	{
		map.clear();
		auto bmap = base->getMap();

		finalAtlasSurf = SDL_CreateRGBSurfaceWithFormat(0, base->getMapSize(), base->getMapSize(), 4, SDL_PIXELFORMAT_RGBA8888);
		for(auto elem : bmap) {
			auto ep = elem.first;
			auto er = elem.second.r;

			auto itr = collection.find(ep);
			if(itr!=collection.end()) {
				map.insert({ep, er});
				AtlasImage::blitWithPadding(itr->second, finalAtlasSurf, er.x, er.y);
			}
		}
	}

	//Build ‘glTexture1’
	glGenTextures(1, &id);
	bind(unit, id);
	AtlasImage::buildGLTexture(finalAtlasSurf);
    unbind();
	SDL_FreeSurface(finalAtlasSurf);
	for(auto& kv : collection) { SDL_FreeSurface(kv.second); }
}
void Atlas::buildFromDirs(const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes, GLuint slot)
{
	//Collect images to be placed in the atlas...
	auto collection = AtlasImage::collectFromDirs(dirPaths, prefixes, true);

	//Build the final atlas surface...
	SDL_Surface* finalAtlasSurf;
	{
		map.clear();
		map = AtlasImage::buildSquareAtlas(collection, mapSize);

		finalAtlasSurf = SDL_CreateRGBSurfaceWithFormat(0, mapSize, mapSize, 4, SDL_PIXELFORMAT_RGBA8888);
		for(auto elem : map) {
			auto ep = elem.first;
			auto er = elem.second.r;

			auto itr = collection.find(ep);
			assert(itr!=collection.end());
			AtlasImage::blitWithPadding(itr->second, finalAtlasSurf, er.x, er.y);
		}
	}

	//Build ‘glTexture1’
	glGenTextures(1, &id);
	bind(unit, id);
	AtlasImage::buildGLTexture(finalAtlasSurf);
    unbind();
	SDL_FreeSurface(finalAtlasSurf);
	for(auto& kv : collection) { SDL_FreeSurface(kv.second); }
}
void Atlas::buildFromSDL_Surface(SDL_Surface* surf, GLuint slot)
{
	//Build ‘glTexture1’
	glGenTextures(1, &id);
	bind(unit, id);
	AtlasImage::buildGLTexture(surf);
    unbind();
}
void Atlas::buildFromImg(std::string imgPath, GLuint slot)
{
	//Initial image load onto SDL surface
	SDL_Surface* imgSurf = IMG_Load(imgPath.c_str());
	if(imgSurf==NULL) {
		Log::errorv(__PRETTY_FUNCTION__, "IMG Error", IMG_GetError());
		return;
	}
	buildFromSDL_Surface(imgSurf, slot);
	SDL_FreeSurface(imgSurf);
}
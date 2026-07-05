#include "Atlas.h"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <assert.h>
#include <cstring>
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
		const Entry& e = itr->second;
		FRect fr = FRect(e.r.r.x+AtlasImage::PAD_F, e.r.r.y+AtlasImage::PAD_F, e.r.r.w-2*AtlasImage::PAD_F, e.r.r.h-2*AtlasImage::PAD_F);
		fr.scale(1.0f/mapSize);
		fr.translate((float)e.page, 0.0f);
		return fr;
	}

	throw std::runtime_error(nch::cat("Key \"", imgID, "\" does not exist within this atlas"));
}
std::map<std::string, Atlas::Entry> Atlas::getMap() {
	return map;
}
int Atlas::getMapSize() {
	return mapSize;
}
int Atlas::getPageCount() {
	return pageCount;
}

void Atlas::bind(GLenum unit, GLuint id)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
}
void Atlas::bind() {
	bind(unit, id);
}
void Atlas::unbind() {
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
void Atlas::texUnit(Shader* shader, const char* uniform, GLuint unit)
{
    GLuint texUni = glGetUniformLocation(shader->getID(), uniform);
    shader->useProgram();
    glUniform1i(texUni, unit);
}
void Atlas::saveDump(const std::string& imgPath)
{
	if(id==0 || mapSize<=0) {
		Log::warnv(__PRETTY_FUNCTION__, "doing nothing", "Atlas has no dumpable pages (id=%u, mapSize=%d)", id, mapSize);
		return;
	}

	//Read back every layer at once (GL_RGBA byte order == SDL_PIXELFORMAT_ABGR8888, matching upload)
	size_t pageBytes = (size_t)mapSize*mapSize*4;
	std::vector<unsigned char> pixels(pageBytes*pageCount);
	bind();
	glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	unbind();

	for(int p = 0; p<pageCount; p++) {
		std::string path = imgPath;
		if(pageCount>1) {
			std::string suffix = nch::cat("_p", p);
			size_t dot = path.find_last_of('.');
			if(dot==std::string::npos) path += suffix;
			else path.insert(dot, suffix);
		}

		SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(pixels.data()+p*pageBytes, mapSize, mapSize, 32, mapSize*4, SDL_PIXELFORMAT_ABGR8888);
		IMG_SavePNG(surf, path.c_str());
		SDL_FreeSurface(surf);
	}
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
	pageCount = 1;
	anims.clear();
	lastAnimMS = 0;
	built = false;
}

int Atlas::maxPageSizeOverride = 0;
void Atlas::setMaxPageSizeOverride(int px) {
	maxPageSizeOverride = px;
}
int Atlas::animFPS = 25;
void Atlas::setAnimFPS(int fps) {
	animFPS = fps<1 ? 1 : fps;
}
int Atlas::resolveMaxPageSize()
{
	GLint glMax = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMax);
	if(glMax<=0) {
		Log::warnv(__PRETTY_FUNCTION__, "assuming 4096", "Could not query GL_MAX_TEXTURE_SIZE (no GL context?)");
		glMax = 4096;
	}

	int ret = glMax;
	if(maxPageSizeOverride>0 && maxPageSizeOverride<ret) ret = maxPageSizeOverride;
	return ret;
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
	std::map<std::string, AtlasImage::AnimSpec> animSpecs;
	auto collection = AtlasImage::collectFromDirs(dirPaths, prefixes, true, &animSpecs);

	//Build the final atlas page surfaces...
	std::vector<SDL_Surface*> pageSurfs;
	{
		map.clear();
		auto bmap = base->getMap();

		//Variant shares the base's layout, dimensions, and paging; mirror them so saveDump/getSrc work
		//and one set of UVs addresses both atlases.
		mapSize = base->getMapSize();
		pageCount = base->getPageCount();
		for(int p = 0; p<pageCount; p++)
			pageSurfs.push_back(SDL_CreateRGBSurfaceWithFormat(0, mapSize, mapSize, 4, SDL_PIXELFORMAT_RGBA8888));
		for(auto elem : bmap) {
			auto ep = elem.first;
			auto er = elem.second.r.r;

			auto itr = collection.find(ep);
			if(itr!=collection.end()) {
				map.insert({ep, elem.second});
				AtlasImage::blitWithPadding(itr->second, pageSurfs[elem.second.page], er.x, er.y);
			}
		}
	}

	//Build ‘glTexture1’
	glGenTextures(1, &id);
	bind(unit, id);
	AtlasImage::buildGLTextureArray(pageSurfs);
    unbind();
	buildAnims(animSpecs);
	for(auto ps : pageSurfs) SDL_FreeSurface(ps);
	for(auto& kv : collection) { SDL_FreeSurface(kv.second); }
}
void Atlas::buildFromDirs(const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes, GLuint slot)
{
	//Collect images to be placed in the atlas...
	std::map<std::string, AtlasImage::AnimSpec> animSpecs;
	auto collection = AtlasImage::collectFromDirs(dirPaths, prefixes, true, &animSpecs);

	//Build the final atlas page surfaces...
	std::vector<SDL_Surface*> pageSurfs;
	{
		map.clear();
		map = AtlasImage::buildPagedAtlas(collection, resolveMaxPageSize(), mapSize, pageCount);
		if(pageCount>1) Log::log("Atlas \"%s\" spans %d pages of %dx%d.", dirPaths[0].c_str(), pageCount, mapSize, mapSize);

		for(int p = 0; p<pageCount; p++)
			pageSurfs.push_back(SDL_CreateRGBSurfaceWithFormat(0, mapSize, mapSize, 4, SDL_PIXELFORMAT_RGBA8888));
		for(auto elem : map) {
			auto ep = elem.first;
			auto er = elem.second.r.r;

			auto itr = collection.find(ep);
			assert(itr!=collection.end());
			AtlasImage::blitWithPadding(itr->second, pageSurfs[elem.second.page], er.x, er.y);
		}
	}

	//Build ‘glTexture1’
	glGenTextures(1, &id);
	bind(unit, id);
	AtlasImage::buildGLTextureArray(pageSurfs);
    unbind();
	buildAnims(animSpecs);
	for(auto ps : pageSurfs) SDL_FreeSurface(ps);
	for(auto& kv : collection) { SDL_FreeSurface(kv.second); }
}
void Atlas::buildFromSDL_Surface(SDL_Surface* surf, GLuint slot)
{
	//Build ‘glTexture1’ (a single-layer array, so every Atlas binds/samples uniformly)
	pageCount = 1;
	glGenTextures(1, &id);
	bind(unit, id);
	AtlasImage::buildGLTextureArray({surf});
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
void Atlas::buildAnims(std::map<std::string, AtlasImage::AnimSpec>& specs)
{
	const int PAD = AtlasImage::PAD;
	for(auto& kv : specs) {
		AtlasImage::AnimSpec& spec = kv.second;

		auto it = map.find(kv.first);
		if(it!=map.end()) {
			const SDL_Rect& cell = it->second.r.r;
			int innerW = cell.w-2*PAD, innerH = cell.h-2*PAD;

			Anim a;
			a.r = it->second.r;
			a.page = it->second.page;
			a.fps = spec.fps;
			a.loop = spec.loop;

			for(SDL_Surface* fs : spec.frames) {
				if(fs->w!=innerW || fs->h!=innerH) {
					Log::warnv(__PRETTY_FUNCTION__, "skipping frame", "Animation \"%s\" frame is %dx%d but base image is %dx%d.", kv.first.c_str(), fs->w, fs->h, innerW, innerH);
					continue;
				}

				//Replicate the packed cell exactly (image + PAD edge bleed), then store GL-ready ABGR bytes.
				SDL_Surface* padded = SDL_CreateRGBSurfaceWithFormat(0, cell.w, cell.h, 32, SDL_PIXELFORMAT_RGBA8888);
				SDL_FillRect(padded, NULL, 0);
				AtlasImage::blitWithPadding(fs, padded, 0, 0);

				SDL_Surface* conv = SDL_ConvertSurfaceFormat(padded, SDL_PIXELFORMAT_ABGR8888, 0);
				SDL_FreeSurface(padded);
				if(conv==NULL) continue;

				std::vector<uint8_t> bytes((size_t)cell.w*cell.h*4);
				for(int row = 0; row<cell.h; row++)
					std::memcpy(bytes.data()+(size_t)row*cell.w*4, (uint8_t*)conv->pixels+(size_t)row*conv->pitch, (size_t)cell.w*4);
				SDL_FreeSurface(conv);

				a.frameBytes.push_back(std::move(bytes));
			}

			if(!a.frameBytes.empty()) anims.push_back(std::move(a));
		}

		for(SDL_Surface* fs : spec.frames) SDL_FreeSurface(fs);
		spec.frames.clear();
	}
}
void Atlas::animFrames()
{
	if(anims.empty()) return;

	//animFPS gate: skip the scan (and GL bind) entirely between upload windows.
	uint64_t now = Timer::getTicks();
	uint64_t gateMS = 1000ULL/(uint64_t)(animFPS<1 ? 1 : animFPS);
	if(now<lastAnimMS+gateMS) return;
	lastAnimMS = now;

	bind();
	for(Anim& a : anims) {
		int n = (int)a.frameBytes.size();
		if(n==0) continue;

		int f = (int)((now*(uint64_t)a.fps)/1000ULL);
		f = a.loop ? (f%n) : std::min(f, n-1);
		if(f==a.lastFrame) continue;
		a.lastFrame = f;

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, a.r.r.x, a.r.r.y, a.page, a.r.r.w, a.r.r.h, 1, GL_RGBA, GL_UNSIGNED_BYTE, a.frameBytes[f].data());
	}
	unbind();
}
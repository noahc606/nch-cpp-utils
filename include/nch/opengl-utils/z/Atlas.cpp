#include "Atlas.h"
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <nch/cpp-utils/filepath.h>
#include <nch/cpp-utils/fs-utils.h>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/timer.h>
#include "nch/opengl-utils/z/atlas/MaxRectsBin.h"

using namespace nch;

Atlas::Atlas(Atlas* base, std::string path, GLuint slot)
{
	switch(slot) {
		case 0: { type = "diffuse"; } break;
		case 1: { type = "specular"; } break;
	}
    unit = slot;
    
	//Assume building from files within dir
	if(FsUtils::dirExists(path)) {
		if(base==nullptr) buildFromDir(path, slot);
		if(base!=nullptr) buildVariantFromDir(base, path, slot);
		return;
	}
	//Assume building from single image file
	if(FsUtils::fileExists(path)) {
		buildFromImg(path, slot);
		return;
	}
	throw std::invalid_argument(Log::getFormattedString("Could not resolve object \"%s\" as a file or dir", path.c_str()));
}
Atlas::Atlas(SDL_Surface* surf, GLuint slot)
{
	switch(slot) {
		case 0: { type = "diffuse"; } break;
		case 1: { type = "specular"; } break;
	}
    unit = slot;
	buildFromSDL_Surface(surf, slot);
}
Atlas::Atlas(std::string path, GLuint slot):
Atlas(nullptr, path, slot){}

Atlas::~Atlas()
{
	if(id!=0)
    	glDeleteTextures(1, &id);
}

GLuint Atlas::getID() {
	return id;
}
const std::string& Atlas::getType() {
    return type;
}
FRect Atlas::getSrc(const std::string& imgID) {
	auto itr = map.find(imgID);
	if(itr!=map.end()) {
		FRect fr = FRect(itr->second.r.x, itr->second.r.y, itr->second.r.w, itr->second.r.h);
		fr.scale(1.0f/mapSize);
		return fr;
	}

	Log::error(__PRETTY_FUNCTION__, "Key \"%s\" does not exist within this atlas", imgID.c_str());
	throw std::invalid_argument("");
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

std::map<std::string, SDL_Surface*> Atlas::collectImagesFromDir(std::string dirPath)
{
	std::map<std::string, SDL_Surface*> ret;

	FsUtils::ListSettings lise; lise.excludeSymlinkDirs = true; lise.includeHiddenEntries = false; lise.maxItemsToList = 99999;
	FsUtils::RecursionSettings rese; rese.recursiveSearch = true;
	auto dir = FsUtils::getDirContents(dirPath, lise, rese);
	for(std::string obj : dir) {
		SDL_Surface* rawSurf = IMG_Load(obj.c_str());
		if(rawSurf==NULL) continue;
		SDL_Surface* convSurf = SDL_ConvertSurfaceFormat(rawSurf, SDL_PIXELFORMAT_RGBA8888, 0);
		SDL_FreeSurface(rawSurf);

		if(convSurf->w>512 || convSurf->h>512) {
			Log::warnv(__PRETTY_FUNCTION__, "skipping entry", "Image \"%s\" is too large (max 512x512) to be added to this atlas.", obj.c_str());
			continue;
		}
		if(convSurf==NULL) {
			Log::warnv(__PRETTY_FUNCTION__, "skipping entry", "Failed to IMG_Load() and SDL_ConvertSurfaceFormat() for \"%s\"", obj.c_str());
			continue;
		}

		std::string atlasID = FilePath(obj).getObjectName(false);
		ret.insert({atlasID, convSurf});
	}
	return ret;
}
void Atlas::buildVariantFromDir(Atlas* base, std::string dirPath, GLuint slot)
{
	//Collect images to be placed in the atlas...
	auto collection = collectImagesFromDir(dirPath);

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
				SDL_BlitSurface(itr->second, NULL, finalAtlasSurf, &er);
			}
		}
	}

	//Build 'glTexture1'
	glGenTextures(1, &id);
	bind(unit, id);
	buildGL_TextureFromSDL_Surface(finalAtlasSurf);
    unbind();
	SDL_FreeSurface(finalAtlasSurf);
}
void Atlas::buildFromDir(std::string dirPath, GLuint slot)
{
	//Collect images to be placed in the atlas...
	auto collection = collectImagesFromDir(dirPath);

	//Build the final atlas surface...
	SDL_Surface* finalAtlasSurf;
	{
		map.clear();
		map = buildSquareAtlas(collection, mapSize);

		finalAtlasSurf = SDL_CreateRGBSurfaceWithFormat(0, mapSize, mapSize, 4, SDL_PIXELFORMAT_RGBA8888);
		for(auto elem : map) {
			auto ep = elem.first;
			auto er = elem.second.r;

			auto itr = collection.find(ep);
			assert(itr!=collection.end());
			SDL_BlitSurface(itr->second, NULL, finalAtlasSurf, &er);
		}
	}

	//Build 'glTexture1'
	glGenTextures(1, &id);
	bind(unit, id);
	buildGL_TextureFromSDL_Surface(finalAtlasSurf);
    unbind();
	SDL_FreeSurface(finalAtlasSurf);
}
void Atlas::buildFromSDL_Surface(SDL_Surface* surf, GLuint slot)
{
	//Build 'glTexture1'
	glGenTextures(1, &id);
	bind(unit, id);
	buildGL_TextureFromSDL_Surface(surf);
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
void Atlas::buildGL_TextureFromSDL_Surface(SDL_Surface* imgSurf)
{
	//Anisotropy params
	GLfloat maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	//Set wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	//set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//Set filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	/* Create GL texture after converting SDL surface to the proper pixel format */
	{
		SDL_Surface* finalSurf = imgSurf;
		switch(imgSurf->format->BytesPerPixel) {
			case 1: {
				//Create a 1-channel OpenGL texture (spectral maps)...
				finalSurf = imgSurf;
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, finalSurf->w, finalSurf->h, 0, GL_RED, GL_UNSIGNED_BYTE, finalSurf->pixels);
			} break;
			case 3: case 4: {
				//Create a 4-channel OpenGL texture...
				finalSurf = SDL_ConvertSurfaceFormat(imgSurf, SDL_PIXELFORMAT_ABGR8888, 0);
				if(finalSurf==NULL) {
					Log::errorv(__PRETTY_FUNCTION__, "IMG Error", IMG_GetError());
					return;
				}
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, finalSurf->w, finalSurf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, finalSurf->pixels);
			} break;
			default: {
				Log::error(__PRETTY_FUNCTION__, "Failed conversion (provided 'imgSurf' must be a surface with 1, 3, or 4 bytes per pixel).");
				throw std::invalid_argument("");
			}
		}

		//Cleanup
		if(imgSurf!=finalSurf) SDL_FreeSurface(finalSurf);
	}
	
	//Generate texture mipmap
	glGenerateMipmap(GL_TEXTURE_2D);
}
std::map<std::string, Rect> Atlas::buildSquareAtlas(const std::map<std::string, SDL_Surface*>& collection, int& outSize) {
    std::map<std::string, Rect> ret;
	
	std::vector<Atlas::ImageInfo> images; {
		images.reserve(collection.size());
		for (auto& kv : collection)
			images.push_back({ kv.first, kv.second, kv.second->w, kv.second->h });

		//Sort largest first (better packing)
		std::sort(images.begin(), images.end(), [](const ImageInfo& a, const ImageInfo& b) {
			return std::max(a.w, a.h) > std::max(b.w, b.h);
		});
	}

	int totalArea, maxDim; {
		totalArea = 0;
		maxDim = 0;
		for(auto& img : images) {
			totalArea += img.w*img.h;
			maxDim = std::max({maxDim, img.w, img.h});
		}
	}

	int low, high; {
		low = maxDim;
		high = std::max(low*2, (int)std::ceil(std::sqrt(totalArea))*2);
		while(low<high) {
			int mid = (low+high)/2;
			std::map<std::string, Rect> temp;
			if(tryPackMaxRects(mid, images, temp)) {
				ret = std::move(temp);
				high = mid; //try smaller
			} else {
				low = mid+1; //need bigger square
			}
		}
	}


    outSize = high;
    return ret;
}
bool Atlas::tryPackMaxRects(int size, const std::vector<Atlas::ImageInfo>& images, std::map<std::string, Rect>& atlas) {
    MaxRectsBin bin(size, size);
    atlas.clear();

    for (const auto& img : images) {
        Rect r;
        if(!bin.insert({img.w, img.h}, r))
            return false; // doesn’t fit
        atlas[img.name] = r;
    }
    return true;
}

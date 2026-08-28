#include "AtlasImage.h"
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <nch/cpp-utils/color.h>
#include <nch/cpp-utils/filepath.h>
#include <nch/cpp-utils/fs-utils.h>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/string-utils.h>
#include <nch/json-utils/json.h>
#include <set>
#include <stdexcept>
#include "MaxRectsBin.h"
using namespace nch;

std::map<std::string, SDL_Surface*> AtlasImage::collectFromPaths(const std::vector<std::vector<std::string>>& objCollections, const std::vector<std::string>& collectionPrefixes, bool jsonFiles, const std::vector<std::string>& collectionRoots, std::map<std::string, AnimSpec>* outAnims) {
	if(objCollections.size()!=collectionPrefixes.size()) throw std::invalid_argument("Expected number of 'objCollections' to == number of prefixes");

	//Flatten every (collection, path) pair so one loop covers all files
	std::vector<std::pair<size_t, const std::string*>> items;
	for(size_t i = 0; i<objCollections.size(); i++)
		for(const std::string& obj : objCollections[i]) items.push_back({i, &obj});

	std::vector<std::string> prefixes(collectionPrefixes.size());
	for(size_t i = 0; i<prefixes.size(); i++) {
		prefixes[i] = nch::cat(StringUtils::trimmed(collectionPrefixes[i], "/"), "/");
		if(prefixes[i]=="/") prefixes[i] = "";
	}

	//Roots must be cleaned the same way FilePath cleans each object path below, or a caller-supplied
	//root spelled with a doubled slash ("bin//assets/...") fails to prefix-match its own files. The
	//subdirectory part of the key would then silently vanish, collapsing "dir/img" onto "img" and
	//dropping every deeper entry as a duplicate.
	std::vector<std::string> roots(collectionRoots.size());
	for(size_t i = 0; i<roots.size(); i++) roots[i] = StringUtils::trimmed(FilePath(collectionRoots[i]).get(), "/");

	//Decode/composite in parallel (PNG decode dominates); results land in per-item slots.
	//IMG_Init up front: IMG_Load lazily initializes format handlers, which must not race.
	//A wildcard JSON contributes several images, so each slot holds a list rather than one image.
	struct Loaded {
		std::string key;
		SDL_Surface* surf = nullptr;
		AnimSpec anim;
		bool hasAnim = false;
	};
	std::vector<std::vector<Loaded>> loaded(items.size());
	IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);

	#pragma omp parallel for schedule(dynamic)
	for(size_t n = 0; n<items.size(); n++) {
		size_t i = items[n].first;
		const std::string& obj = *items[n].second;

		FilePath fp(obj);
		bool isJson = jsonFiles && fp.getExtension()=="json";

		//Keep subdirectory structure (relative to the collection root) within the key
		std::string relDirs = "";
		if(i<roots.size() && !roots[i].empty()) {
			std::string parent = StringUtils::trimmed(fp.getParentDirPath(), "/");
			const std::string& root = roots[i];
			if(parent.rfind(root, 0)==0) {
				relDirs = StringUtils::trimmed(parent.substr(root.size()), "/");
				if(!relDirs.empty()) relDirs += "/";
			}
		}
		std::string keyPrefix = nch::cat(prefixes[i], relDirs);

		if(!isJson) {
			SDL_Surface* rawSurf = IMG_Load(obj.c_str());
			if(rawSurf==NULL) continue;
			SDL_Surface* surf = SDL_ConvertSurfaceFormat(rawSurf, SDL_PIXELFORMAT_ABGR8888, 0);
			SDL_FreeSurface(rawSurf);
			if(!isSurfaceAtlasable(surf, obj)) { SDL_FreeSurface(surf); continue; }

			Loaded ld;
			ld.key = keyPrefix+fp.getObjectName(false);
			ld.surf = surf;
			loaded[n].push_back(std::move(ld));
			continue;
		}

		//Parsed once and reused: both the composite and the animation block read the same doc.
		const nlohmann::json doc = JSON::loadFromFile(obj);
		std::string jsonDir = fp.getParentDirPath();

		std::vector<std::string> names;
		nlohmann::json docs;
		if(!expandWildcards(doc, obj, names, docs)) {
			names.push_back(fp.getObjectName(false));
			docs = nlohmann::json::array();
			docs.push_back(doc);
		}

		for(size_t k = 0; k<names.size(); k++) {
			SDL_Surface* surf = buildSurfaceFromDoc(docs[k], jsonDir, obj);
			if(!isSurfaceAtlasable(surf, obj)) { SDL_FreeSurface(surf); continue; }

			Loaded ld;
			ld.key = keyPrefix+names[k];
			ld.surf = surf;
			if(outAnims!=nullptr) ld.hasAnim = parseAnimation(docs[k], jsonDir, obj, ld.anim);
			loaded[n].push_back(std::move(ld));
		}
	}

	//Merge in original order (first key wins, matching the old serial loop); dropped duplicates are freed
	std::map<std::string, SDL_Surface*> ret;
	for(std::vector<Loaded>& group : loaded) {
		for(Loaded& ld : group) {
			if(ld.surf==nullptr) continue;
			if(!ret.insert({ld.key, ld.surf}).second) {
				SDL_FreeSurface(ld.surf);
				for(SDL_Surface* fs : ld.anim.frames) SDL_FreeSurface(fs);
				continue;
			}
			if(ld.hasAnim && outAnims!=nullptr) outAnims->insert({ld.key, ld.anim});
		}
	}
	return ret;
}
std::map<std::string, SDL_Surface*> AtlasImage::collectFromDirs(const std::vector<std::string>& dirPaths, const std::vector<std::string>& prefixes, bool jsonFiles, std::map<std::string, AnimSpec>* outAnims) {
	FsUtils::ListSettings lise; lise.excludeSymlinkDirs = true; lise.includeHiddenEntries = false; lise.maxItemsToList = 100000;
	FsUtils::RecursionSettings rese; rese.recursiveSearch = true;

	std::vector<std::vector<std::string>> manyDirConts; manyDirConts.reserve(dirPaths.size());
	for(size_t i = 0; i<dirPaths.size(); i++) {
		const auto& vec = (FsUtils::getDirContents(dirPaths[i], lise, rese));
		manyDirConts.push_back(vec);
	}

	return collectFromPaths(manyDirConts, prefixes, jsonFiles, dirPaths, outAnims);
}
std::map<std::string, SDL_Surface*> AtlasImage::collectFromDir(const std::string& dirPath, const std::string& prefix, bool jsonFiles)
{
	FsUtils::ListSettings lise; lise.excludeSymlinkDirs = true; lise.includeHiddenEntries = false; lise.maxItemsToList = 100000;
	FsUtils::RecursionSettings rese; rese.recursiveSearch = true;
	auto dirConts = FsUtils::getDirContents(dirPath, lise, rese);

	return collectFromPaths({dirConts}, {prefix}, jsonFiles, {dirPath});
}

bool AtlasImage::expandWildcards(const nlohmann::json& doc, const std::string& srcPath, std::vector<std::string>& outNames, nlohmann::json& outDocs)
{
	const nlohmann::json& wcs = JSON::getObject(doc, "wildcards");
	if(wcs.empty()) return false;

	std::string nameTemplate = JSON::getOpt(doc, "names", "");
	if(nameTemplate.empty()) {
		Log::errorv(__PRETTY_FUNCTION__, "skipping", "\"%s\" declares \"wildcards\" but no \"names\" template", srcPath.c_str());
		return false;
	}

	//An axis is either a bare token list or a token -> properties map; props stay pointers into 'doc'.
	std::vector<std::string> axisNames;
	std::vector<std::vector<std::string>> axisTokens;
	std::vector<std::vector<const nlohmann::json*>> axisProps;
	for(auto& kv : wcs.items()) {
		std::vector<std::string> tokens;
		std::vector<const nlohmann::json*> props;

		if(kv.value().is_array()) {
			for(auto& tok : kv.value()) {
				tokens.push_back(tok.is_string() ? tok.get<std::string>() : tok.dump());
				props.push_back(nullptr);
			}
		} else if(kv.value().is_object()) {
			for(auto& tokKV : kv.value().items()) {
				tokens.push_back(tokKV.key());
				props.push_back(&tokKV.value());
			}
		} else {
			Log::errorv(__PRETTY_FUNCTION__, "skipping", "\"%s\": wildcard \"%s\" must be an array of tokens or a token->properties object", srcPath.c_str(), kv.key().c_str());
			return false;
		}

		if(tokens.empty()) {
			Log::errorv(__PRETTY_FUNCTION__, "skipping", "\"%s\": wildcard \"%s\" declares no tokens", srcPath.c_str(), kv.key().c_str());
			return false;
		}
		axisNames.push_back(kv.key());
		axisTokens.push_back(tokens);
		axisProps.push_back(props);
	}

	size_t total = 1;
	for(const std::vector<std::string>& toks : axisTokens) {
		//Divided rather than multiplied so the check itself cannot overflow
		if(toks.size()>(size_t)MAX_WILDCARD_EXPANSIONS/total) {
			Log::errorv(__PRETTY_FUNCTION__, "skipping", "\"%s\": wildcards expand past the %d-image cap", srcPath.c_str(), (int)MAX_WILDCARD_EXPANSIONS);
			return false;
		}
		total *= toks.size();
	}

	std::vector<std::string> names;
	nlohmann::json docs = nlohmann::json::array();
	std::set<std::string> seen;

	//Odometer over the axes, least-significant last
	std::vector<size_t> idx(axisTokens.size(), 0);
	for(size_t c = 0; c<total; c++) {
		nlohmann::json subs = nlohmann::json::object();
		for(size_t a = 0; a<axisNames.size(); a++) {
			subs[axisNames[a]] = axisTokens[a][idx[a]];
			const nlohmann::json* p = axisProps[a][idx[a]];
			if(p!=nullptr && p->is_object()) {
				for(auto& propKV : p->items()) subs[axisNames[a]+"."+propKV.key()] = propKV.value();
			}
		}

		std::string name = substitutedString(nameTemplate, subs, srcPath);
		if(!seen.insert(name).second) {
			Log::warnv(__PRETTY_FUNCTION__, "skipping duplicate", "\"%s\": \"names\" yields \"%s\" more than once; does the template use every wildcard?", srcPath.c_str(), name.c_str());
		} else {
			nlohmann::json resolved = doc;
			resolved.erase("wildcards");
			resolved.erase("names");
			substitutePlaceholders(resolved, subs, srcPath);

			names.push_back(name);
			docs.push_back(std::move(resolved));
		}

		for(size_t a = axisNames.size(); a-->0; ) {
			if(++idx[a]<axisTokens[a].size()) break;
			idx[a] = 0;
		}
	}

	if(names.empty()) return false;
	outNames = std::move(names);
	outDocs = std::move(docs);
	return true;
}
void AtlasImage::substitutePlaceholders(nlohmann::json& node, const nlohmann::json& subs, const std::string& srcPath)
{
	if(node.is_object() || node.is_array()) {
		for(auto& child : node) substitutePlaceholders(child, subs, srcPath);
		return;
	}
	if(!node.is_string()) return;

	//A string that is nothing but one placeholder adopts the substitution's own type, which is the
	//only way an array-valued property ("colormod") survives; anything else interpolates as text.
	std::string s = node.get<std::string>();
	if(s.size()>2 && s.front()=='{' && s.back()=='}' && s.find('{', 1)==std::string::npos && s.find('}')==s.size()-1) {
		auto itr = subs.find(s.substr(1, s.size()-2));
		if(itr!=subs.end()) { node = *itr; return; }
	}
	node = substitutedString(s, subs, srcPath);
}
std::string AtlasImage::substitutedString(const std::string& s, const nlohmann::json& subs, const std::string& srcPath)
{
	std::string out;
	size_t pos = 0;
	while(pos<s.size()) {
		size_t open = s.find('{', pos);
		size_t close = (open==std::string::npos) ? std::string::npos : s.find('}', open);
		if(close==std::string::npos) { out += s.substr(pos); break; }

		out += s.substr(pos, open-pos);
		std::string key = s.substr(open+1, close-open-1);
		auto itr = subs.find(key);
		if(itr==subs.end()) {
			Log::warnv(__PRETTY_FUNCTION__, "leaving as-is", "\"%s\": unknown wildcard placeholder \"{%s}\"", srcPath.c_str(), key.c_str());
			out += s.substr(open, close-open+1);
		} else {
			out += itr->is_string() ? itr->get<std::string>() : itr->dump();
		}
		pos = close+1;
	}
	return out;
}
SDL_Surface* AtlasImage::buildSurfaceFromDoc(const nlohmann::json& doc, const std::string& jsonDir, const std::string& srcPath)
{
	const nlohmann::json& elems = JSON::getArray(doc, "applied_elements");
	if(elems.empty()) {
		Log::errorv(__PRETTY_FUNCTION__, "skipping", "JSON \"%s\" missing \"applied_elements\" array", srcPath.c_str());
		return nullptr;
	}

	return compositeFromElements(elems, jsonDir, srcPath);
}
bool AtlasImage::isSurfaceAtlasable(SDL_Surface* surf, const std::string& srcPath)
{
	if(surf==NULL) return false;
	if(surf->w>512 || surf->h>512) {
		Log::warnv(__PRETTY_FUNCTION__, "skipping entry", "Image \"%s\" is too large (max 512x512) to be added to this atlas.", srcPath.c_str());
		return false;
	}
	return true;
}
SDL_Surface* AtlasImage::compositeFromElements(const nlohmann::json& appliedElems, const std::string& jsonDir, const std::string& srcPath)
{
	//Determine composite dimensions from first img
	int w = 0, h = 0;
	for(auto& elem : appliedElems) {
		std::string img = JSON::getOpt(elem, "img", "");
		if(img.empty()) continue;
		SDL_Surface* s = IMG_Load((jsonDir+"/"+img).c_str());
		if(s) { w = s->w; h = s->h; SDL_FreeSurface(s); break; }
	}
	if(w==0 || h==0) {
		Log::errorv(__PRETTY_FUNCTION__, "skipping", "JSON \"%s\" has no \"applied_elements\" entry with a loadable \"img\" to take dimensions from (paths resolve against \"%s\")",
			srcPath.c_str(), jsonDir.c_str());
		return nullptr;
	}

	SDL_Surface* composite = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
	SDL_FillRect(composite, NULL, 0);

	for(auto& elem : appliedElems) {
		//Parse colormod
		Uint8 cr=255, cg=255, cb=255, ca=255;
		if(JSON::has(elem, "colormod") && !parseColormod(elem.at("colormod"), cr, cg, cb, ca)) {
			Log::errorv(__PRETTY_FUNCTION__, "skipping file", "JSON \"%s\": \"colormod\" must be an array of 3 or 4 ints, or a 6- or 8-digit hex string",
				srcPath.c_str());
			SDL_FreeSurface(composite);
			return nullptr;
		}

		SDL_Surface* layer;
		std::string img = JSON::getOpt(elem, "img", "");
		if(!img.empty()) {
			std::string imgPath = jsonDir+"/"+img;
			SDL_Surface* rawLayer = IMG_Load(imgPath.c_str());
			if(!rawLayer) {
				Log::warnv(__PRETTY_FUNCTION__, "skipping element", "Failed to load \"%s\"", imgPath.c_str());
				continue;
			}
			layer = SDL_ConvertSurfaceFormat(rawLayer, SDL_PIXELFORMAT_ABGR8888, 0);
			SDL_FreeSurface(rawLayer);
		} else {
			layer = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
			SDL_FillRect(layer, NULL, SDL_MapRGBA(layer->format, 255, 255, 255, 255));
		}

		if(layer==NULL) continue;

		layer = mirrorSurface(layer, JSON::getOpt(elem, "mirror_h", false), JSON::getOpt(elem, "mirror_v", false));
		layer = rotateSurface(layer, JSON::getOpt(elem, "rotate_cw", 0)-JSON::getOpt(elem, "rotate_ccw", 0));

		SDL_SetSurfaceColorMod(layer, cr, cg, cb);
		SDL_SetSurfaceAlphaMod(layer, ca);
		SDL_SetSurfaceBlendMode(layer, SDL_BLENDMODE_BLEND);

		SDL_Rect destRect = {0, 0, w, h};
		SDL_BlitScaled(layer, NULL, composite, &destRect);
		SDL_FreeSurface(layer);
	}

	return composite;
}
bool AtlasImage::parseColormod(const nlohmann::json& cm, Uint8& outR, Uint8& outG, Uint8& outB, Uint8& outA)
{
	if(cm.is_string()) {
		std::string hex = cm.get<std::string>();
		if(!hex.empty() && hex[0]=='#') hex = hex.substr(1);
		//Color::fromStringB16 silently drops anything non-hex, so stray characters are rejected here instead
		if(hex.size()!=6 && hex.size()!=8) return false;
		if(hex.find_first_not_of("0123456789ABCDEFabcdef")!=std::string::npos) return false;

		Color c = Color::fromStringB16(hex);
		outR = c.r; outG = c.g; outB = c.b; outA = c.a;
		return true;
	}

	if(!cm.is_array()) return false;
	std::vector<int> vals;
	try {
		vals = JSON::toList<int>(cm);
	} catch(const std::exception&) { return false; }
	if(vals.size()!=3 && vals.size()!=4) return false;

	outR = (Uint8)vals[0];
	outG = (Uint8)vals[1];
	outB = (Uint8)vals[2];
	outA = (vals.size()==4) ? (Uint8)vals[3] : 255;
	return true;
}
SDL_Surface* AtlasImage::mirrorSurface(SDL_Surface* src, bool mirrorH, bool mirrorV)
{
	if(!mirrorH && !mirrorV) return src;

	SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h, src->format->BitsPerPixel, src->format->format);
	if(dst==NULL) return src;

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	int bpp = src->format->BytesPerPixel;
	for(int y = 0; y<src->h; y++) {
		int sy = mirrorV ? (src->h-1-y) : y;
		Uint8* srcRow = (Uint8*)src->pixels+sy*src->pitch;
		Uint8* dstRow = (Uint8*)dst->pixels+y*dst->pitch;
		for(int x = 0; x<src->w; x++) {
			int sx = mirrorH ? (src->w-1-x) : x;
			memcpy(dstRow+x*bpp, srcRow+sx*bpp, bpp);
		}
	}
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);

	SDL_FreeSurface(src);
	return dst;
}
SDL_Surface* AtlasImage::rotateSurface(SDL_Surface* src, int numTurnsCW)
{
	int turns = ((numTurnsCW%4)+4)%4;
	if(turns==0) return src;

	bool swapsDims = (turns==1 || turns==3);
	int dw = swapsDims ? src->h : src->w;
	int dh = swapsDims ? src->w : src->h;
	SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, dw, dh, src->format->BitsPerPixel, src->format->format);
	if(dst==NULL) return src;

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	int bpp = src->format->BytesPerPixel;
	for(int y = 0; y<dh; y++) {
		Uint8* dstRow = (Uint8*)dst->pixels+y*dst->pitch;
		for(int x = 0; x<dw; x++) {
			//Source pixel landing on (x, y) once rotated
			int sx, sy;
			switch(turns) {
				case 1:  sx = y;           sy = src->h-1-x; break;
				case 2:  sx = src->w-1-x;  sy = src->h-1-y; break;
				default: sx = src->w-1-y;  sy = x;          break;
			}
			memcpy(dstRow+x*bpp, (Uint8*)src->pixels+sy*src->pitch+sx*bpp, bpp);
		}
	}
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);

	SDL_FreeSurface(src);
	return dst;
}
bool AtlasImage::parseAnimation(const nlohmann::json& doc, const std::string& jsonDir, const std::string& srcPath, AnimSpec& out)
{
	const nlohmann::json& anim = JSON::getObject(doc, "animation");
	const nlohmann::json& frames = JSON::getArray(anim, "frames");
	if(frames.empty()) return false;

	out.fps = std::max(1, JSON::getOpt(anim, "fps", out.fps));
	out.loop = JSON::getOpt(anim, "loop", out.loop);

	for(auto& frame : frames) {
		//A frame is either a bare "img.png" string or a full { "applied_elements": [...] } object.
		SDL_Surface* surf = nullptr;
		if(frame.is_string()) {
			nlohmann::json elem; elem["img"] = frame.get<std::string>();
			surf = compositeFromElements(nlohmann::json::array({elem}), jsonDir, srcPath);
		} else {
			surf = compositeFromElements(JSON::getArray(frame, "applied_elements"), jsonDir, srcPath);
		}

		if(surf!=nullptr) out.frames.push_back(surf);
	}

	return !out.frames.empty();
}

void AtlasImage::buildGLTexture(SDL_Surface* imgSurf)
{
	//Anisotropy params
	GLfloat maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	//Set wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
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
void AtlasImage::buildGLTextureArray(const std::vector<SDL_Surface*>& pages)
{
	assert(!pages.empty());
	int w = pages[0]->w, h = pages[0]->h;

	//Anisotropy params
	GLfloat maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	//Set wrapping parameters
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//Set filtering parameters
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* Allocate all layers, then upload each page after converting to the proper pixel format */
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, w, h, (GLsizei)pages.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	for(size_t i = 0; i<pages.size(); i++) {
		SDL_Surface* imgSurf = pages[i];
		assert(imgSurf->w==w && imgSurf->h==h);

		SDL_Surface* finalSurf = imgSurf;
		switch(imgSurf->format->BytesPerPixel) {
			case 1: {
				//Upload a 1-channel layer (spectral maps)...
				glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, (GLint)i, w, h, 1, GL_RED, GL_UNSIGNED_BYTE, finalSurf->pixels);
			} break;
			case 3: case 4: {
				//Upload a 4-channel layer (pages built as ABGR8888 upload without conversion)...
				if(imgSurf->format->format!=SDL_PIXELFORMAT_ABGR8888) {
					finalSurf = SDL_ConvertSurfaceFormat(imgSurf, SDL_PIXELFORMAT_ABGR8888, 0);
					if(finalSurf==NULL) {
						Log::errorv(__PRETTY_FUNCTION__, "IMG Error", IMG_GetError());
						return;
					}
				}
				glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, (GLint)i, w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, finalSurf->pixels);
			} break;
			default: {
				Log::error(__PRETTY_FUNCTION__, "Failed conversion (each page must be a surface with 1, 3, or 4 bytes per pixel).");
				throw std::invalid_argument("");
			}
		}

		//Cleanup
		if(imgSurf!=finalSurf) SDL_FreeSurface(finalSurf);
	}

	//Generate texture mipmap
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}
std::map<std::string, Rect> AtlasImage::buildSquareAtlas(const std::map<std::string, SDL_Surface*>& collection, int& outSize)
{
	std::map<std::string, Rect> ret;
	std::vector<AtlasImage> images = sortedBySize(collection);

	int64_t totalArea; int maxDim; {
		totalArea = 0;
		maxDim = 0;
		for(auto& img : images) {
			totalArea += (int64_t)(img.w+2*PAD)*(img.h+2*PAD);
			maxDim = std::max({maxDim, img.w+2*PAD, img.h+2*PAD});
		}
	}

	int low, high; {
		//No packing can beat the total-area bound, so the search starts there instead of at
		//the largest single image (saves several full re-packs on dense collections).
		int areaBound = (int)std::ceil(std::sqrt((double)totalArea));
		low = std::max(maxDim, areaBound);
		high = std::max(low*2, areaBound*2);
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
std::map<std::string, AtlasImage::Entry> AtlasImage::buildPagedAtlas(const std::map<std::string, SDL_Surface*>& collection, int maxPageSize, int& outSize, int& outPageCount)
{
	std::map<std::string, Entry> ret;

	//A collection fitting within one page keeps the shrink-to-fit behavior of buildSquareAtlas
	std::map<std::string, Rect> single = buildSquareAtlas(collection, outSize);
	if(outSize<=maxPageSize) {
		outPageCount = 1;
		for(auto& kv : single) ret.insert({kv.first, {kv.second, 0}});
		return ret;
	}

	//Too big for one page: every page is maxPageSize wide, overflow spills largest-first onto new pages
	outSize = maxPageSize;
	std::vector<AtlasImage> remaining = sortedBySize(collection);
	int page = 0;
	while(!remaining.empty()) {
		MaxRectsBin bin(outSize, outSize);
		std::vector<AtlasImage> overflow;
		for(auto& img : remaining) {
			Rect r;
			if(bin.insert({img.w+2*PAD, img.h+2*PAD}, r)) {
				ret.insert({img.name, {r, page}});
			} else {
				overflow.push_back(img);
			}
		}
		if(overflow.size()==remaining.size())
			throw std::runtime_error(nch::cat("Image \"", overflow[0].name, "\" cannot fit within a ", maxPageSize, "x", maxPageSize, " atlas page"));
		remaining = std::move(overflow);
		page++;
	}

	outPageCount = page;
	return ret;
}
void AtlasImage::blitWithPadding(SDL_Surface* src, SDL_Surface* dst, int dstX, int dstY)
{
	int imgW = src->w, imgH = src->h;
	SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);

	//Main image
	SDL_Rect dstImg = {dstX+PAD, dstY+PAD, imgW, imgH};
	SDL_BlitSurface(src, NULL, dst, &dstImg);

	//Left edge
	SDL_Rect srcL = {0, 0, 1, imgH};
	SDL_Rect dstL = {dstX, dstY+PAD, PAD, imgH};
	SDL_BlitScaled(src, &srcL, dst, &dstL);

	//Right edge
	SDL_Rect srcR = {imgW-1, 0, 1, imgH};
	SDL_Rect dstR = {dstX+PAD+imgW, dstY+PAD, PAD, imgH};
	SDL_BlitScaled(src, &srcR, dst, &dstR);

	//Top edge
	SDL_Rect srcT = {0, 0, imgW, 1};
	SDL_Rect dstT = {dstX+PAD, dstY, imgW, PAD};
	SDL_BlitScaled(src, &srcT, dst, &dstT);

	//Bottom edge
	SDL_Rect srcB = {0, imgH-1, imgW, 1};
	SDL_Rect dstB = {dstX+PAD, dstY+PAD+imgH, imgW, PAD};
	SDL_BlitScaled(src, &srcB, dst, &dstB);

	//Top-left corner
	SDL_Rect srcTL = {0, 0, 1, 1};
	SDL_Rect dstTL = {dstX, dstY, PAD, PAD};
	SDL_BlitScaled(src, &srcTL, dst, &dstTL);

	//Top-right corner
	SDL_Rect srcTR = {imgW-1, 0, 1, 1};
	SDL_Rect dstTR = {dstX+PAD+imgW, dstY, PAD, PAD};
	SDL_BlitScaled(src, &srcTR, dst, &dstTR);

	//Bottom-left corner
	SDL_Rect srcBL = {0, imgH-1, 1, 1};
	SDL_Rect dstBL = {dstX, dstY+PAD+imgH, PAD, PAD};
	SDL_BlitScaled(src, &srcBL, dst, &dstBL);

	//Bottom-right corner
	SDL_Rect srcBR = {imgW-1, imgH-1, 1, 1};
	SDL_Rect dstBR = {dstX+PAD+imgW, dstY+PAD+imgH, PAD, PAD};
	SDL_BlitScaled(src, &srcBR, dst, &dstBR);
}
std::vector<AtlasImage> AtlasImage::sortedBySize(const std::map<std::string, SDL_Surface*>& collection)
{
	std::vector<AtlasImage> images;
	images.reserve(collection.size());
	for(auto& kv : collection)
		images.push_back({ kv.first, kv.second, kv.second->w, kv.second->h });

	//Sort largest first (better packing)
	std::sort(images.begin(), images.end(), [](const AtlasImage& a, const AtlasImage& b) {
		return std::max(a.w, a.h) > std::max(b.w, b.h);
	});
	return images;
}
bool AtlasImage::tryPackMaxRects(int size, const std::vector<AtlasImage>& images, std::map<std::string, Rect>& atlas)
{
	MaxRectsBin bin(size, size);
	atlas.clear();

	for(const auto& img : images) {
		Rect r;
		if(!bin.insert({img.w+2*PAD, img.h+2*PAD}, r))
			return false; //doesn't fit
		atlas[img.name] = r;
	}
	return true;
}

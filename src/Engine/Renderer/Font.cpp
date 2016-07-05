using namespace std;
#include <stdint.h>
#include <string>
#include "Font.h"

const char *DEFAULT_FONT = "Arial_Narrow";

static const float old_scale2 = 0.6f;
static const float old_scale1 = 0.3f;

__inline float Old_SelectValueForScale(float scale, float v0, float v1, float v2) {
	return (scale >= old_scale2 ? v2 : (scale >= old_scale1 ? v1 : v0));
}

Font *Font::RemapFont(const char *baseName) {
	string cleanName = baseName;
	if (cleanName == DEFAULT_FONT)
		return nullptr;
	const char *remapped = Localization::FindString("#font_" + cleanName);
	if (remapped != nullptr)
		return renderSystem->RegisterFont(remapped);
	const char *wildcard = Localization::FindString("#font_*");
	if (wildcard != nullptr && cleanName.Icmp(wildcard) != 0)
		return renderSystem->RegisterFont(wildcard);
	// Note single | so both sides are always executed
	if (cleanName.ReplaceChar(' ', '_') | cleanName.ReplaceChar('-', '_'))
		return renderSystem->RegisterFont(cleanName);
	return nullptr;
}

Font::~Font() {
	delete _fontInfo;
}

Font::Font(const char *name) : _name(name) {
	_fontInfo = nullptr;
	_alias = RemapFont(name);

	if (_alias) {
		// Make sure we don't have a circular reference
		for (Font *f = _alias; f != nullptr; f = f->_alias)
			if (f == this)
				idLib::FatalError("Font alias \"%s\" is a circular reference!", name);
		return;
	}

	if (!LoadFont()) {
		if (name.Icmp(DEFAULT_FONT) == 0)
			idLib::FatalError("Could not load default font \"%s\"", DEFAULT_FONT);
		else {
			idLib::Warning("Could not load font %s", name);
			alias = renderSystem->RegisterFont(DEFAULT_FONT);
		}
	}
}

struct oldGlyphInfo_t {
	int height;			// number of scan lines
	int top;			// top of glyph in buffer
	int bottom;			// bottom of glyph in buffer
	int pitch;			// width for copying
	int xSkip;			// x adjustment
	int imageWidth;		// width of actual image
	int imageHeight;	// height of actual image
	float s;			// x offset in image where glyph starts
	float t;			// y offset in image where glyph starts
	float s2;
	float t2;
	int junk;
	char materialName[32];
};
static const int GLYPHS_PER_FONT = 256;

bool LoadOldGlyphData(const char *filename, oldGlyphInfo_t glyphInfo[GLYPHS_PER_FONT]) {
	File *fd = fileSystem->OpenFileRead(filename);
	if (!fd)
		return false;
	fd->Read(glyphInfo, GLYPHS_PER_FONT * sizeof(oldGlyphInfo_t));
	for (int i = 0; i < GLYPHS_PER_FONT; i++) {
		idSwap::Little(glyphInfo[i].height);
		idSwap::Little(glyphInfo[i].top);
		idSwap::Little(glyphInfo[i].bottom);
		idSwap::Little(glyphInfo[i].pitch);
		idSwap::Little(glyphInfo[i].xSkip);
		idSwap::Little(glyphInfo[i].imageWidth);
		idSwap::Little(glyphInfo[i].imageHeight);
		idSwap::Little(glyphInfo[i].s);
		idSwap::Little(glyphInfo[i].t);
		idSwap::Little(glyphInfo[i].s2);
		idSwap::Little(glyphInfo[i].t2);
		assert(glyphInfo[i].imageWidth == glyphInfo[i].pitch);
		assert(glyphInfo[i].imageHeight == glyphInfo[i].height);
		assert(glyphInfo[i].imageWidth == (glyphInfo[i].s2 - glyphInfo[i].s) * 256);
		assert(glyphInfo[i].imageHeight == (glyphInfo[i].t2 - glyphInfo[i].t) * 256);
		assert(glyphInfo[i].junk == 0);
	}
	delete fd;
	return true;
}

bool Font::LoadFont() {
	string fontName = va("newfonts/%s/48.dat", GetName());
	idFile *fd = fileSystem->OpenFileRead(fontName);
	if (!fd)
		return false;

	const int FONT_INFO_VERSION = 42;
	const int FONT_INFO_MAGIC = (FONT_INFO_VERSION | ('i' << 24) | ('d' << 16) | ('f' << 8));

	uint32 version = 0;
	fd->ReadBig(version);
	if (version != FONT_INFO_MAGIC) {
		idLib::Warning("Wrong version in %s", GetName());
		delete fd;
		return false;
	}
	_fontInfo = new (TAG_FONT) fontInfo_t;
	short pointSize = 0;
	fd->ReadBig(pointSize);
	assert(pointSize == 48);
	fd->ReadBig(fontInfo->ascender);
	fd->ReadBig(fontInfo->descender);
	fd->ReadBig(fontInfo->numGlyphs);
	_fontInfo->glyphData = (glyphInfo_t *)Mem_Alloc(sizeof(glyphInfo_t) * _fontInfo->numGlyphs, TAG_FONT);
	_fontInfo->charIndex = (uint32 *)Mem_Alloc(sizeof(uint32) * _fontInfo->numGlyphs, TAG_FONT);
	fd->Read(fontInfo->glyphData, _fontInfo->numGlyphs * sizeof(glyphInfo_t));

	for (int i = 0; i < fontInfo->numGlyphs; i++) {
		idSwap::Little(fontInfo->glyphData[i].width);
		idSwap::Little(fontInfo->glyphData[i].height);
		idSwap::Little(fontInfo->glyphData[i].top);
		idSwap::Little(fontInfo->glyphData[i].left);
		idSwap::Little(fontInfo->glyphData[i].xSkip);
		idSwap::Little(fontInfo->glyphData[i].s);
		idSwap::Little(fontInfo->glyphData[i].t);
	}

	fd->Read(fontInfo->charIndex, fontInfo->numGlyphs * sizeof(uint32));
	idSwap::LittleArray(fontInfo->charIndex, fontInfo->numGlyphs);

	memset(fontInfo->ascii, -1, sizeof(fontInfo->ascii));
	for (int i = 0; i < fontInfo->numGlyphs; i++) {
		if (fontInfo->charIndex[i] < 128) {
			fontInfo->ascii[fontInfo->charIndex[i]] = i;
		}
		else {
			// Since the characters are sorted, as soon as we find a non-ascii character, we can stop
			break;
		}
	}

	string fontTextureName = fontName;
	fontTextureName.SetFileExtension("tga");

	_fontInfo->material = declManager->FindMaterial(fontTextureName);
	_fontInfo->material->SetSort(SS_GUI);

	// Load the old glyph data because we want our new fonts to fit in the old glyph metrics
	int pointSizes[3] = { 12, 24, 48 };
	float scales[3] = { 4.0f, 2.0f, 1.0f };
	for (int i = 0; i < 3; i++) {
		oldGlyphInfo_t oldGlyphInfo[GLYPHS_PER_FONT];
		const char *oldFileName = va("newfonts/%s/old_%d.dat", GetName(), pointSizes[i]);
		if (LoadOldGlyphData(oldFileName, oldGlyphInfo)) {
			int mh = 0;
			int mw = 0;
			for (int g = 0; g < GLYPHS_PER_FONT; g++) {
				if (mh < oldGlyphInfo[g].height) {
					mh = oldGlyphInfo[g].height;
				}
				if (mw < oldGlyphInfo[g].xSkip)
					mw = oldGlyphInfo[g].xSkip;
			}
			_fontInfo->oldInfo[i].maxWidth = scales[i] * mw;
			_fontInfo->oldInfo[i].maxHeight = scales[i] * mh;
		}
		else {
			int mh = 0;
			int mw = 0;
			for (int g = 0; g < fontInfo->numGlyphs; g++) {
				if (mh < fontInfo->glyphData[g].height)
					mh = fontInfo->glyphData[g].height;
				if (mw < fontInfo->glyphData[g].xSkip)
					mw = fontInfo->glyphData[g].xSkip;
			}
			_fontInfo->oldInfo[i].maxWidth = mw;
			_fontInfo->oldInfo[i].maxHeight = mh;
		}
	}
	delete fd;
	return true;
}

int	Font::GetGlyphIndex(uint32 idx) const {
	if (idx < 128)
		return _fontInfo->ascii[idx];
	if (_fontInfo->numGlyphs == 0)
		return -1;
	if (!_fontInfo->charIndex)
		return idx;
	int len = _fontInfo->numGlyphs;
	int mid = _fontInfo->numGlyphs;
	int offset = 0;
	while (mid > 0) {
		mid = len >> 1;
		if (_fontInfo->charIndex[offset + mid] <= idx)
			offset += mid;
		len -= mid;
	}
	return (_fontInfo->charIndex[offset] == idx ? offset : -1);
}

float Font::GetLineHeight(float scale) const {
	if (_alias)
		return _alias->GetLineHeight(scale);
	if (_fontInfo)
		return scale * Old_SelectValueForScale(scale, _fontInfo->oldInfo[0].maxHeight, _fontInfo->oldInfo[1].maxHeight, _fontInfo->oldInfo[2].maxHeight);
	return 0.0f;
}

float Font::GetAscender(float scale) const {
	if (_alias)
		return _alias->GetAscender(scale);
	if (_fontInfo)
		return scale * _fontInfo->ascender;
	return 0.0f;
}

float Font::GetMaxCharWidth(float scale) const {
	if (_alias)
		return _alias->GetMaxCharWidth(scale);
	if (_fontInfo)
		return scale * Old_SelectValueForScale(scale, _fontInfo->oldInfo[0].maxWidth, _fontInfo->oldInfo[1].maxWidth, _fontInfo->oldInfo[2].maxWidth);
	return 0.0f;
}

float Font::GetGlyphWidth(float scale, uint32 idx) const {
	if (_alias)
		return _alias->GetGlyphWidth(scale, idx);
	if (_fontInfo) {
		int i = GetGlyphIndex(idx);
		const int asterisk = 42;
		if (i == -1 && idx != asterisk)
			i = GetGlyphIndex(asterisk);
		if (i >= 0)
			return scale * _fontInfo->glyphData[i].xSkip;
	}
	return 0.0f;
}

void Font::GetScaledGlyph(float scale, uint32 idx, scaledGlyphInfo_t &glyphInfo) const {
	if (_alias)
		return _alias->GetScaledGlyph(scale, idx, glyphInfo);
	if (_fontInfo) {
		int i = GetGlyphIndex(idx);
		const int asterisk = 42;
		if (i == -1 && idx != asterisk)
			i = GetGlyphIndex(asterisk);
		if (i >= 0) {
			float invMaterialWidth = 1.0f / _fontInfo->material->GetImageWidth();
			float invMaterialHeight = 1.0f / _fontInfo->material->GetImageHeight();
			glyphInfo_t &gi = _fontInfo->glyphData[i];
			glyphInfo.xSkip = scale * gi.xSkip;
			glyphInfo.top = scale * gi.top;
			glyphInfo.left = scale * gi.left;
			glyphInfo.width = scale * gi.width;
			glyphInfo.height = scale * gi.height;
			glyphInfo.s1 = (gi.s - 0.5f) * invMaterialWidth;
			glyphInfo.t1 = (gi.t - 0.5f) * invMaterialHeight;
			glyphInfo.s2 = (gi.s + gi.width + 0.5f) * invMaterialWidth;
			glyphInfo.t2 = (gi.t + gi.height + 0.5f) * invMaterialHeight;
			glyphInfo.material = _fontInfo->material;
			return;
		}
	}
	memset(&glyphInfo, 0, sizeof(glyphInfo));
}

void Font::Touch() {
	if (_alias)
		_alias->Touch();
	if (_fontInfo) {
		const_cast<Material *>(_fontInfo->material)->EnsureNotPurged();
		_fontInfo->material->SetSort(SS_GUI);
	}
}

#ifndef __FONT_H__
#define __FONT_H__

struct scaledGlyphInfo_t {
	float top, left;
	float width, height;
	float xSkip;
	float s1, t1, s2, t2;
	const class Material *material;
};

class Font {
public:
	Font(const char *name);
	~Font();

	void Touch();
	const char *GetName() const { return _name.c_str(); }

	float GetLineHeight(float scale) const;
	float GetAscender(float scale) const;
	float GetMaxCharWidth(float scale) const;

	float GetGlyphWidth(float scale, uint32 idx) const;
	void GetScaledGlyph(float scale, uint32 idx, scaledGlyphInfo_t &glyphInfo) const;

private:
	static Font *RemapFont(const char *baseName);
	int	GetGlyphIndex(uint32 idx) const;
	bool LoadFont();

	struct glyphInfo_t {
		uint8 width;	// width of glyph in pixels
		uint8 height;	// height of glyph in pixels
		char top;		// distance in pixels from the base line to the top of the glyph
		char left;		// distance in pixels from the pen to the left edge of the glyph
		uint8 xSkip;	// x adjustment after rendering this glyph
		uint16 s;		// x offset in image where glyph starts (in pixels)
		uint16 t;		// y offset in image where glyph starts (in pixels)
	};
	struct fontInfo_t {
		struct oldInfo_t {
			float maxWidth;
			float maxHeight;
		} oldInfo[3];

		short ascender;
		short descender;

		short numGlyphs;
		glyphInfo_t *glyphData;

		// This is a sorted array of all characters in the font
		// This maps directly to glyphData, so if charIndex[0] is 42 then glyphData[0] is character 42
		uint32 *charIndex;

		// As an optimization, provide a direct mapping for the ascii character set
		char ascii[128];

		const Material *material;
	};

	// base name of the font (minus "fonts/" and ".dat")
	string _name;

	// Fonts can be aliases to other fonts
	Font *_alias;

	// If the font is NOT an alias, this is where the font data is located
	fontInfo_t *_fontInfo;
};

#endif

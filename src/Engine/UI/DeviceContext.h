#ifndef __DEVICECONTEXT_H__
#define __DEVICECONTEXT_H__
#include <vector>
#include <vec3.hpp>
#include <mat3x3.hpp>

// device context support for gui stuff
#include "Rectangle.h"
#include "../renderer/Font.h"

const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 480;
const int BLINK_DIVISOR = 200;

class Material;
class Winding;
class DeviceContext
{
public:
	DeviceContext();
	~DeviceContext() { }

	void Init();
	void Shutdown();
	bool Initialized() { return initialized; }
	void EnableLocalization();

	void GetTransformInfo(vec3 &origin, mat3 &mat);

	void SetTransformInfo(const vec3 &origin, const mat3 &mat);
	void DrawMaterial(float x, float y, float w, float h, const Material *mat, const vec4 &color, float scalex = 1.0, float scaley = 1.0);
	void DrawRect(float x, float y, float width, float height, float size, const vec4 &color);
	void DrawFilledRect(float x, float y, float width, float height, const vec4 &color);
	int DrawText(const char *text, float textScale, int textAlign, vec4 color, Rectangle rectDraw, bool wrap, int cursor = -1, bool calcOnly = false, vector<int> *breaks = NULL, int limit = 0);
	void DrawMaterialRect(float x, float y, float w, float h, float size, const Material *mat, const vec4 &color);
	void DrawStretchPic(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const Material *mat);
	void DrawMaterialRotated(float x, float y, float w, float h, const Material *mat, const vec4 &color, float scalex = 1.0, float scaley = 1.0, float angle = 0.0f);
	void DrawStretchPicRotated(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const Material *mat, float angle = 0.0f);
	void DrawWinding(Winding & w, const Material *mat);

	int CharWidth(const char c, float scale);
	int	TextWidth(const char *text, float scale, int limit);
	int TextHeight(const char *text, float scale, int limit);
	int	MaxCharHeight(float scale);
	int	MaxCharWidth(float scale);

	//Region *GetTextRegion(const char *text, float textScale, Rectangle rectDraw, float xStart, float yStart);

	void SetSize(float width, float height);
	void SetOffset(float x, float y);

	const Material *GetScrollBarImage(int index);

	void DrawCursor(float *x, float *y, float size);
	void SetCursor(int n);

	// clipping rects
	virtual bool ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2);
	virtual void PushClipRect(Rectangle r);
	virtual void PopClipRect();
	virtual void EnableClipping(bool b);

	void SetFont(Font * font) { activeFont = font; }
	void SetOverStrike(bool b) { overStrikeMode = b; }
	bool GetOverStrike() { return overStrikeMode; }
	void DrawEditCursor(float x, float y, float scale);

	enum {
		CURSOR_ARROW,
		CURSOR_HAND,
		CURSOR_HAND_JOY1,
		CURSOR_HAND_JOY2,
		CURSOR_HAND_JOY3,
		CURSOR_HAND_JOY4,
		CURSOR_COUNT
	};

	enum {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

	enum {
		SCROLLBAR_HBACK,
		SCROLLBAR_VBACK,
		SCROLLBAR_THUMB,
		SCROLLBAR_RIGHT,
		SCROLLBAR_LEFT,
		SCROLLBAR_UP,
		SCROLLBAR_DOWN,
		SCROLLBAR_COUNT
	};

	static vec4 colorPurple;
	static vec4 colorOrange;
	static vec4 colorYellow;
	static vec4 colorGreen;
	static vec4 colorBlue;
	static vec4 colorRed;
	static vec4 colorWhite;
	static vec4 colorBlack;
	static vec4 colorNone;

protected:
	virtual int	DrawText(float x, float y, float scale, vec4 color, const char *text, float adjust, int limit, int style, int cursor = -1);
	void PaintChar(float x, float y, const scaledGlyphInfo_t &glyphInfo);
	void Clear();

	const Material *cursorImages[CURSOR_COUNT];
	const Material *scrollBarImages[SCROLLBAR_COUNT];
	const Material *whiteImage;
	Font *activeFont;

	float xScale;
	float yScale;
	float xOffset;
	float yOffset;

	int cursor;

	vector<Rectangle> clipRects;

	bool enableClipping;
	bool overStrikeMode;

	mat3 mat;
	bool matIsIdentity;
	vec3 origin;
	bool initialized;
};

class DeviceContextOptimized : public DeviceContext
{
	virtual bool ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2);
	virtual void PushClipRect(Rectangle r);
	virtual void PopClipRect();
	virtual void EnableClipping(bool b);

	virtual int DrawText(float x, float y, float scale, vec4 color, const char *text, float adjust, int limit, int style, int cursor = -1);

	float clipX1;
	float clipX2;
	float clipY1;
	float clipY2;
};

#endif /* !__DEVICECONTEXT_H__ */

#pragma once

#include <Uxtheme.h>
#include <math.h>
#include <vector>
#include <memory>

#define JPEG_QUALITY_SUPER		0
#define JPEG_QUALITY_GOOD		1
#define JPEG_QUALITY_NORMAL		2
#define JPEG_QUALITY_AVERAGE	3
#define JPEG_QUALITY_BAD		4

class CTxDib
{
	LPRGBQUAD _bits;
	int _width;
	int _height;
	BYTE _maxAlpha;
public:
	CTxDib();
	CTxDib(LPCWSTR fileName);
	CTxDib(CTxDib &val);
	virtual ~CTxDib();

	void operator=(CTxDib &val);

	BOOL Load(LPCWSTR fileName);
	BOOL Load(HRSRC hRes, HMODULE hModule = NULL);
	BOOL Load(LPBYTE data, DWORD size);
	BOOL SavePNG(LPCWSTR fileName);
	BOOL SaveJPG(LPCWSTR fileName, int quality = JPEG_QUALITY_GOOD);
	BOOL SaveBMP(LPCWSTR fileName);

	BOOL Destroy();
	BOOL Draw(HDC hdc, int x, int y, int cx = -1, long cy = -1);
	BOOL Draw(HDC hdc, LPCRECT rcDraw);
	BOOL CreateFromHBITMAP(HBITMAP bmp);
	BOOL CreateFromHICON(HICON ico);

	HBITMAP CreateBitmap(HDC hdc = NULL);
	void SetTransColor(COLORREF clr);
	void Resample( int newWidth, int newHeight, CTxDib *dst = NULL );
	void Tile(HDC hdc, LPRECT rcDraw, LPRECT rcClip = NULL);
	int GetWidth();
	int GetHeight();
	void Crop(int left, int top, int right, int bottom, CTxDib *dst = NULL);
	void Crop(LPCRECT rcCrop, CTxDib *dst = NULL);
	BOOL IsValid();
	void SetMaxAlpha(BYTE alpha);
	BYTE GetMaxAlpha();
	void Colorize(COLORREF clr);
	BOOL CalcAlpha(CTxDib *imgWhite, CTxDib *imgBlack);
	LPRGBQUAD GetBits() { return _bits; }
	void PreMulRGBA(RGBQUAD& color);
	void RotateLeft(CTxDib *dst = NULL);
	void RotateRight(CTxDib *dst = NULL);
	void _copy(LPRGBQUAD newBits, int newWidth, int newHeight, BOOL copyBits = FALSE);
	BOOL Attach(LPVOID dib);
	void PreMultiplyWithAlpha();
private:
	void OverflowCoordinates(float &x, float &y);
	RGBQUAD GetPixelColorWithOverflow(long x, long y);
	RGBQUAD GetAreaColorInterpolated(float const xc, float const yc, float const w, float const h);
	void AddAveragingCont(RGBQUAD const &color, float const surf, float &rr, float &gg, float &bb, float &aa);
	RGBQUAD GetPixelColorInterpolated(float x, float y);
	void Resample2(int newWidth, int newHeight, CTxDib *dst = NULL);
	bool QIShrink(int newWidth, int newHeight, CTxDib *dst = NULL);

	void _copy(CTxDib &val);
};

class CTxDibSet
{
	typedef std::vector<CTxDib*> imgCols;

	std::vector<imgCols> _items;
	int _width;
	int _height;
	int _cols;
	int _rows;
public:
	CTxDibSet(CTxDib *img, int rows, int cols);
	~CTxDibSet();
	CTxDib *Get(int col, int row) { return _items[row][col]; }

	int Width() { return _width; }
	int Height() { return _height; }
	int Rows() { return _rows; }
	int Cols() { return _cols; }
};

class CTxSkinDIB
{
	CTxDib _dibLeftTop;
	CTxDib _dibTop;
	CTxDib _dibRightTop;

	CTxDib _dibLeftCenter;
	CTxDib _dibCenter;
	CTxDib _dibRightCenter;

	CTxDib _dibLeftBottom;
	CTxDib _dibBottom;
	CTxDib _dibRightBottom;

	MARGINS	_margins;
	BOOL _tileX;
	BOOL _tileY;
public:
	CTxSkinDIB();
	virtual ~CTxSkinDIB();

	BOOL Load(LPCWSTR fileName, MARGINS *mg, BOOL tileX, BOOL tileY);
	BOOL Load(CTxDib *dib, MARGINS *mg, BOOL tileX, BOOL tileY);

	void Draw(HDC hdc, LPRECT rcDraw, LPRECT rcClip);
};

//***bd*** simple floating point point
class CTxDibPoint2
{
public:
	CTxDibPoint2();
	CTxDibPoint2(float const x_, float const y_);
	CTxDibPoint2(CTxDibPoint2 const &p);

	float Distance(CTxDibPoint2 const p2);
	float Distance(float const x_, float const y_);

	float x, y;
};

//and simple rectangle
class CTxDibRect2
{
public:
	CTxDibRect2();
	CTxDibRect2(float const x1_, float const y1_, float const x2_, float const y2_);
	CTxDibRect2(CTxDibPoint2 const &bl, CTxDibPoint2 const &tr);
	CTxDibRect2(CTxDibRect2 const &p);

	float Surface() const;
	CTxDibRect2 CrossSection(CTxDibRect2 const &r2);
	CTxDibPoint2 Center();
	float Width();
	float Height();

	CTxDibPoint2 botLeft;
	CTxDibPoint2 topRight;
};

inline RGBQUAD CTxDib::GetPixelColorWithOverflow(long x, long y)
{
	if (!(0 <= y && y < _height && 0 <= x &&  x < _width)) {
		x = max(x, 0); 
		x = min(x, _width - 1);
		y = max(y, 0); 
		y = min(y, _height - 1);
	}
	return _bits[y * _width + x];
}

inline void CTxDib::OverflowCoordinates(float &x, float &y)
{
	if (x >= 0 && x < _width && y >= 0 && y < _height)
		return;
	x = max(x, 0); 
	x = min(x, _width - 1);
	y = max(y, 0); 
	y = min(y, _height - 1);
}

inline void	CTxDib::AddAveragingCont(RGBQUAD const &color, float const surf, float &rr, float &gg, float &bb, float &aa)
{
	rr += color.rgbRed * surf;
	gg += color.rgbGreen * surf;
	bb += color.rgbBlue * surf;
	aa += color.rgbReserved	* surf;
}

inline void CTxDib::PreMulRGBA(RGBQUAD& color)
{
	color.rgbRed = (color.rgbRed * color.rgbReserved) / 255;
	color.rgbGreen = (color.rgbGreen * color.rgbReserved) / 255;
	color.rgbBlue = (color.rgbBlue * color.rgbReserved) / 255;
}

inline int CTxDib::GetWidth()
{ 
	return _width;  
}

inline int CTxDib::GetHeight()	
{ 
	return _height; 
}

inline void CTxDib::Crop(LPCRECT rcCrop, CTxDib *dst /*= NULL*/) 
{ 
	Crop(rcCrop->left, rcCrop->top, rcCrop->right, rcCrop->bottom, dst); 
}

inline BOOL CTxDib::IsValid()
{ 
	return (_bits ? TRUE : FALSE);
}

inline void CTxDib::SetMaxAlpha(BYTE alpha)
{ 
	_maxAlpha = alpha; 
}

inline BYTE CTxDib::GetMaxAlpha()
{ 
	return _maxAlpha;
}

inline BOOL CTxDib::Draw(HDC hdc, LPCRECT rcDraw)
{ 
	return Draw(hdc, rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top); 
}

inline void CTxDib::operator=(CTxDib& val) 
{ 
	_copy(val); 
}

inline CTxDibPoint2::CTxDibPoint2()
{
	x = y = 0.0f;
}

inline CTxDibPoint2::CTxDibPoint2(float const x_, float const y_)
{
	x = x_;
	y = y_;
}

inline CTxDibPoint2::CTxDibPoint2(CTxDibPoint2 const &p)
{
	x = p.x;
	y = p.y;
}

inline float CTxDibPoint2::Distance(CTxDibPoint2 const p2)
{
	return (float)sqrt((x-p2.x)*(x-p2.x)+(y-p2.y)*(y-p2.y));
}

inline float CTxDibPoint2::Distance(float const x_, float const y_)
{
	return (float)sqrt((x-x_)*(x-x_)+(y-y_)*(y-y_));
}

inline CTxDibRect2::CTxDibRect2()
{
}

inline CTxDibRect2::CTxDibRect2(float const x1_, float const y1_, float const x2_, float const y2_)
{
	botLeft.x = x1_;
	botLeft.y = y1_;
	topRight.x = x2_;
	topRight.y = y2_;
}

inline CTxDibRect2::CTxDibRect2(CTxDibRect2 const &p)
{
	botLeft = p.botLeft;
	topRight = p.topRight;
}

// Returns the surface of rectangle.
inline float CTxDibRect2::Surface() const
{
	return (topRight.x-botLeft.x)*(topRight.y-botLeft.y);
}

inline CTxDibRect2 CTxDibRect2::CrossSection(CTxDibRect2 const &r2)
{
	CTxDibRect2 cs;
	cs.botLeft.x = max(botLeft.x, r2.botLeft.x);
	cs.botLeft.y = max(botLeft.y, r2.botLeft.y);
	cs.topRight.x = min(topRight.x, r2.topRight.x);
	cs.topRight.y = min(topRight.y, r2.topRight.y);
	return (cs.botLeft.x <= cs.topRight.x && cs.botLeft.y <= cs.topRight.y ? cs : CTxDibRect2(0,0,0,0));
}

inline CTxDibPoint2 CTxDibRect2::Center()
{
	return CTxDibPoint2((topRight.x+botLeft.x)/2.0f, (topRight.y+botLeft.y)/2.0f);
}

inline float CTxDibRect2::Width()
{
	return topRight.x-botLeft.x;
}

inline float CTxDibRect2::Height()
{
	return topRight.y-botLeft.y;
}


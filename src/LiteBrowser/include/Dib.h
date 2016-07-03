#pragma once
#include <windows.h>

class Dib
{
	HBITMAP _bmp;
	HBITMAP _oldBmp;
	HDC _hdc;
	LPRGBQUAD _bits;
	BOOL _ownData;
	int _width;
	int _height;

	HDC _hTargetDC;
	POINT _oldViewPort;
	BOOL _restoreViewPort;
	RECT _rcTarget;
public:
	Dib();
	~Dib();

	int Width() const { return _width; }
	int Height() const { return _height; }
	HDC Hdc() const { return _hdc; }
	HBITMAP Bmp() const { return _bmp; }
	LPRGBQUAD Bits() const { return _bits; }

	bool Create(int width, int height, bool topdowndib = false);
	bool Create(HDC hdc, HBITMAP bmp, LPRGBQUAD bits, int width, int height);

	void Clear();
	void Destroy(bool del_bmp = true);
	void Draw(HDC hdc, int x, int y);
	void Draw(HDC hdc, LPRECT rcDraw);
	HDC BeginPaint(HDC hdc, LPRECT rcDraw);
	void EndPaint(bool copy = false);
	HBITMAP DetachBitmap();

	operator HDC() { return _hdc; }
};
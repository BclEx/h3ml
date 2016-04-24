#include <Windows.h>
#include "TxDIB.h"
#include <emmintrin.h>
#include <mmintrin.h>
#include "..\..\lib\freeimage\FreeImage.h"

CTxDib::CTxDib()
{
	_maxAlpha = 255;
	_bits = NULL;
	_width = 0;
	_height = 0;
}

CTxDib::CTxDib(CTxDib &val)
{
	_bits = NULL;
	_width = 0;
	_height	= 0;
	_maxAlpha = 255;
	_copy(val);
}

CTxDib::CTxDib(LPCWSTR fileName)
{
	_bits = NULL;
	_width = 0;
	_height	= 0;
	_maxAlpha = 255;
	Load(fileName);
}

CTxDib::~CTxDib()
{
	Destroy();
}

BOOL CTxDib::Load(LPCWSTR fileName)
{
	BOOL ret = FALSE;
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(fileName);
	FIBITMAP *dib = FreeImage_LoadU(fif, fileName);
	ret = Attach(dib);
	PreMultiplyWithAlpha();
	FreeImage_Unload(dib);
	return ret;
}

BOOL CTxDib::Load(HRSRC hRes, HMODULE hModule /*= NULL*/)
{
	BOOL ret = FALSE;
	DWORD rsize	= SizeofResource(hModule, hRes);
	HGLOBAL hMem = LoadResource(hModule, hRes);
	if (hMem) {
		LPBYTE lpData = (LPBYTE) LockResource(hMem);
		ret = Load(lpData, rsize);
	}
	return ret;
}

BOOL CTxDib::Load(LPBYTE data, DWORD size)
{
	BOOL ret = FALSE;
	FIMEMORY *mem = FreeImage_OpenMemory(data, size);
	if (mem) {
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem, size);
		FIBITMAP *dib = FreeImage_LoadFromMemory(fif, mem);
		ret = Attach(dib);
		PreMultiplyWithAlpha();
		FreeImage_Unload(dib);
		FreeImage_CloseMemory(mem);
	}
	return ret;
}

BOOL CTxDib::Destroy()
{
	if (_bits) {
		free(_bits);
		_bits = NULL;
	}
	_width = 0;
	_height = 0;
	return TRUE;
}

BOOL CTxDib::Draw(HDC hdc, int x, int y, int cx /*= -1*/, long cy /*= -1*/)
{
	if (!_bits || !cx || !cy)
		return FALSE;
	if (cx > 0 && cx != _width || cy > 0 && cy != _height) {
		CTxDib newdib;
		Resample(cx, cy, &newdib);
		return newdib.Draw(hdc, x, y);
	}
	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateBitmap(memDC);
	HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = _maxAlpha;
	AlphaBlend(hdc, x, y, (cx >= 0 ? cx : _width), (cy >= 0 ? cy : _height), memDC, 0, 0, _width, _height, bf);
	SelectObject(memDC, oldBmp);
	DeleteObject(bmp);
	DeleteDC(memDC);
	return TRUE;
}

void CTxDib::SetTransColor(COLORREF clr)
{
	RGBQUAD rgb;
	rgb.rgbRed = GetBValue(clr);
	rgb.rgbGreen = GetGValue(clr);
	rgb.rgbBlue = GetBValue(clr);
	if (_bits) {
		size_t cnt = _width * _height;
		for (size_t i = 0; i < cnt; i++)
			if (_bits[i].rgbRed == rgb.rgbRed && _bits[i].rgbGreen == rgb.rgbGreen && _bits[i].rgbBlue == rgb.rgbBlue) {
				_bits[i].rgbReserved = 0;
				PreMulRGBA(_bits[i]);
			}
	}
}

void CTxDib::Resample(int newWidth, int newHeight, CTxDib *dst)
{
	if (newHeight <= 0 || newWidth <= 0 || !IsValid())
		return;
	if (newWidth < _width && newHeight < _height)
		QIShrink(newWidth, newHeight, dst);
	else
		Resample2(newWidth, newHeight, dst);
}

RGBQUAD CTxDib::GetPixelColorInterpolated(float x, float y)
{
	float sX, sY;         //source location
	WORD wt1, wt2, wd, wb, wc, wa;
	WORD wrr, wgg, wbb, waa;
	int xi, yi;
	float t1, t2, d, b, c, a;
	RGBQUAD rgb11,rgb21,rgb12,rgb22;
	RGBQUAD color;
	LPRGBQUAD pxptr;

	yi = (int)y; if (y < 0) yi--;
	xi = (int)x; if (x < 0) xi--;

	wt2 = (WORD)((x - yi) * 256.0f);
	if (xi < -1 || xi >= _width || yi < -1 || yi >= _height) {
		// recalculate coordinates and use faster method later on
		OverflowCoordinates(sX, sY);
		xi = (int)sX; if (sX<0) xi--; // sX and/or sY have changed ... recalculate xi and yi
		yi = (int)sY; if (sY<0) yi--;
		wt2 = (WORD)((sY - yi) * 256.0f);
	}
	// get four neighbouring pixels
	if ((xi + 1) < _width && xi >= 0 && (yi + 1) < _height && yi >= 0) {
		// all pixels are inside RGB24 image... optimize reading (and use fixed point arithmetic)
		wt1 = (WORD)((sX - xi) * 256.0f);
		wd  = wt1 * wt2 >> 8;
		wb  = wt1 - wd;
		wc  = wt2 - wd;
		wa  = 256 - wt1 - wc;

		pxptr = _bits + yi * _width + xi;
		wbb = wa * pxptr->rgbBlue; 
		wgg = wa * pxptr->rgbGreen; 
		wrr = wa * pxptr->rgbRed;
		waa = wa * pxptr->rgbReserved;
		pxptr++;
		wbb += wb * pxptr->rgbBlue; 
		wgg += wb * pxptr->rgbGreen; 
		wrr += wb * pxptr->rgbRed;
		waa += wb * pxptr->rgbReserved;
		pxptr += _width - 1;
		wbb += wc * pxptr->rgbBlue; 
		wgg += wc * pxptr->rgbGreen; 
		wrr += wc * pxptr->rgbRed;
		waa += wc * pxptr->rgbReserved;
		pxptr++;
		wbb += wd * pxptr->rgbBlue; 
		wgg += wd * pxptr->rgbGreen; 
		wrr += wd * pxptr->rgbRed;
		waa += wd * pxptr->rgbReserved;

		color.rgbRed = (BYTE) (wrr >> 8); 
		color.rgbGreen = (BYTE) (wgg >> 8); 
		color.rgbBlue = (BYTE) (wbb >> 8);
		color.rgbReserved = (BYTE) (waa >> 8);
	}
	else {
		// default (slower) way to get pixels (not RGB24 or some pixels out of borders)
		t1 = sX - xi;
		t2 = sY - yi;
		d  = t1 * t2;
		b  = t1 - d;
		c  = t2 - d;
		a  = 1 - t1 - c;
		rgb11 = GetPixelColorWithOverflow(xi, yi);
		rgb21 = GetPixelColorWithOverflow(xi + 1, yi);
		rgb12 = GetPixelColorWithOverflow(xi, yi + 1);
		rgb22 = GetPixelColorWithOverflow(xi + 1, yi + 1);
		// calculate linear interpolation
		color.rgbRed		= (BYTE)(a * rgb11.rgbRed		+ b * rgb21.rgbRed		+ c * rgb12.rgbRed		+ d * rgb22.rgbRed);
		color.rgbGreen		= (BYTE)(a * rgb11.rgbGreen		+ b * rgb21.rgbGreen	+ c * rgb12.rgbGreen	+ d * rgb22.rgbGreen);
		color.rgbBlue		= (BYTE)(a * rgb11.rgbBlue		+ b * rgb21.rgbBlue		+ c * rgb12.rgbBlue		+ d * rgb22.rgbBlue);
		color.rgbReserved	= (BYTE)(a * rgb11.rgbReserved	+ b * rgb21.rgbReserved + c * rgb12.rgbReserved	+ d * rgb22.rgbReserved);
	}
	return color;
}

void CTxDib::PreMultiplyWithAlpha()
{
	if (!IsValid())
		return;
	int cnt = _width * _height;
	for (int i = 0; i < cnt; i++)
		PreMulRGBA(_bits[i]);
}

void CTxDib::_copy(CTxDib &val)
{
	_copy(val._bits, val._width, val._height, TRUE);
	_maxAlpha = val._maxAlpha;
}

void CTxDib::Crop(int left, int top, int right, int bottom, CTxDib *dst /*= NULL*/)
{
	if (!IsValid())
		return;
	left = max(0, min(left, _width));
	top = max(0, min(top, _height));
	right = max(0, min(right, _width));
	bottom = max(0, min(bottom, _height));

	int newWidth = right - left;
	int newHeight = bottom - top;
	if (newWidth <= 0 || newHeight <= 0) {
		if (dst)
			dst->Destroy();
		return;
	}

	LPRGBQUAD newBits = (LPRGBQUAD) malloc(newWidth * newHeight * sizeof(RGBQUAD));
	int startSrc = (_height - bottom) * _width + left;
	int startDst = 0;
	for (int i = 0; i < newHeight; i++, startSrc += _width, startDst += newWidth)
		memcpy(newBits + startDst, _bits + startSrc, newWidth * sizeof(RGBQUAD));

	if (dst) {
		dst->_copy(newBits, newWidth, newHeight);
		dst->_maxAlpha = _maxAlpha;
	}
	else
		_copy(newBits, newWidth, newHeight);
}

void CTxDib::_copy(LPRGBQUAD newBits, int newWidth, int newHeight, BOOL copyBits)
{
	Destroy();
	_width = newWidth;
	_height = newHeight;
	if (!copyBits)
		_bits = newBits;
	else {
		if (newBits) {
			size_t sz = _width * _height * sizeof(RGBQUAD);
			_bits = (LPRGBQUAD) malloc(sz);
			memcpy(_bits, newBits, sz);
		}
		else
			newBits = NULL;
	}
}

void CTxDib::Tile(HDC hdc, LPRECT rcDraw, LPRECT rcClip /*= NULL*/)
{
	if (!_width || !_height || !_bits)
		return;
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha	= 255;

	int x = 0;
	int y = 0;

	HBITMAP bmp = CreateBitmap(hdc);
	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP	oldBmp = (HBITMAP)SelectObject(memDC, bmp);
	RECT rcDst;
	RECT rcTemp;

	int drawWidth = 0;
	int drawHeight = 0;
	int imgX = 0;
	int imgY = 0;

	for (int top = rcDraw->top; top <= rcDraw->bottom; top += _height) {
		for (int left = rcDraw->left; left <= rcDraw->right; left += _width) {
			rcDst.left = left;
			rcDst.top = top;
			rcDst.right = left + _width;
			rcDst.bottom = top + _height;
			if (!rcClip || rcClip && IntersectRect(&rcTemp, &rcDst, rcClip)) {
				imgX = 0;
				imgY = 0;
				drawWidth = _width;
				drawHeight = _height;
				if (rcClip) {
					imgY = rcTemp.top  - rcDst.top;
					imgX = rcTemp.left - rcDst.left;
					drawWidth = rcTemp.right - rcTemp.left;
					drawHeight = rcTemp.bottom - rcTemp.top;
					rcDst = rcTemp;
				}
				drawWidth -= (rcDst.right - rcDraw->right)  <= 0 ? 0 : (rcDst.right - rcDraw->right);
				drawHeight -= (rcDst.bottom - rcDraw->bottom) <= 0 ? 0 : (rcDst.bottom - rcDraw->bottom);
				AlphaBlend(hdc, rcDst.left, rcDst.top, drawWidth, drawHeight, memDC, imgX, imgY, drawWidth, drawHeight, bf);
			}
		}
	}
	SelectObject(memDC, oldBmp);
	DeleteObject(bmp);
	DeleteDC(memDC);
}

HBITMAP CTxDib::CreateBitmap(HDC hdc)
{
	HBITMAP bmp = NULL;
	BITMAPINFO bmp_info = {0}; 
	bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	bmp_info.bmiHeader.biWidth = _width; 
	bmp_info.bmiHeader.biHeight = _height; 
	bmp_info.bmiHeader.biPlanes = 1; 
	bmp_info.bmiHeader.biBitCount = 32; 

	HDC dc = hdc;
	if (!dc)
		dc = GetDC(NULL);

	void *buf = nullptr; 
	bmp = ::CreateDIBSection(hdc, &bmp_info, DIB_RGB_COLORS, &buf, 0, 0); 
	memcpy(buf, _bits, _width * _height * sizeof(RGBQUAD));
	if (!hdc)
		ReleaseDC(NULL, dc);
	return bmp;
}

BOOL CTxDib::CreateFromHBITMAP(HBITMAP bmp)
{
	FIBITMAP *dib = NULL;
	if (bmp) {
		BITMAP bm;
		GetObject(bmp, sizeof(BITMAP), (LPSTR) &bm);
		dib = FreeImage_Allocate(bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
		int colors = FreeImage_GetColorsUsed(dib);
		HDC dc = GetDC(NULL);
		int res = GetDIBits(dc, bmp, 0, FreeImage_GetHeight(dib), FreeImage_GetBits(dib), FreeImage_GetInfo(dib), DIB_RGB_COLORS);
		ReleaseDC(NULL, dc);
		FreeImage_GetInfoHeader(dib)->biClrUsed = colors;
		FreeImage_GetInfoHeader(dib)->biClrImportant = colors;
		Attach(dib);
		FreeImage_Unload(dib);
		return TRUE;
	}
	return FALSE;
}

BOOL CTxDib::Attach(LPVOID pdib)
{
	Destroy();

	FIBITMAP *dib = (FIBITMAP *)pdib;
	if (dib) {
		BOOL delDIB = FALSE;;
		if (FreeImage_GetBPP(dib) != 32) {
			FIBITMAP *dib32 = FreeImage_ConvertTo32Bits(dib);
			if (dib32) {
				dib = dib32;
				delDIB = TRUE;
			}
			else
				dib = NULL;
		}
		if (dib) {
			BITMAPINFO *hdr = FreeImage_GetInfo(dib);
			_width = FreeImage_GetWidth(dib);
			_height = FreeImage_GetHeight(dib);
			_bits = (LPRGBQUAD)malloc(_width * _height * sizeof(RGBQUAD));
			memcpy(_bits, FreeImage_GetBits(dib), _width * _height * sizeof(RGBQUAD));
			if (delDIB)
				FreeImage_Unload(dib);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CTxDib::CreateFromHICON(HICON ico)
{
	Destroy();
	BOOL ret = FALSE;
	if (ico)  { 
		ICONINFO iinfo;
		GetIconInfo(ico, &iinfo);
		if (iinfo.hbmColor) {
			if (CreateFromHBITMAP(iinfo.hbmColor)) {
				CTxDib mask;
				if (mask.CreateFromHBITMAP(iinfo.hbmMask)) {
					int cnt = _width * _height;
					for (int i = 0; i < cnt; i++) {
						if (mask._bits[i].rgbBlue) {
							_bits[i].rgbReserved = 0;
							_bits[i].rgbRed = 0;
							_bits[i].rgbGreen = 0;
							_bits[i].rgbBlue = 0;
						}
						else if (!_bits[i].rgbReserved)
							_bits[i].rgbReserved = 255;
						PreMulRGBA(_bits[i]);
					}
				}
				ret = TRUE;
			}
		}
		else if (iinfo.hbmMask) {
			BITMAP bm;
			GetObject(iinfo.hbmMask, sizeof(BITMAP), (LPSTR) &bm);
			if (bm.bmWidth * 2 == bm.bmHeight) {
				_width = bm.bmWidth;
				_height = bm.bmHeight / 2;
				_bits = (LPRGBQUAD)malloc(_width * _height * sizeof(RGBQUAD));
				ZeroMemory(_bits, _width * _height * sizeof(RGBQUAD));
				CTxDib dib;
				dib.CreateFromHBITMAP(iinfo.hbmMask);
				dib.Crop(0, _width, _width, _height, NULL);
				int cnt = _width * _height;
				for (int i = 0; i < cnt; i++) {
					if (dib._bits[i].rgbBlue) {
						_bits[i].rgbReserved = 255;
						_bits[i].rgbRed = 0;
						_bits[i].rgbGreen = 0;
						_bits[i].rgbBlue = 0;
					}
					else {
						_bits[i].rgbReserved = 0;
						_bits[i].rgbRed = 0;
						_bits[i].rgbGreen = 0;
						_bits[i].rgbBlue = 0;
					}
				}
				ret = TRUE;
			}
		}

		if (iinfo.hbmColor) DeleteObject(iinfo.hbmColor);
		if (iinfo.hbmMask) DeleteObject(iinfo.hbmMask);
	}
	return ret;
}

void CTxDib::Colorize(COLORREF clr)
{
	if (_bits) {
		BYTE r = GetRValue(clr);
		BYTE g = GetGValue(clr);
		BYTE b = GetBValue(clr);
		int cnt = _width * _height;
		for (int i = 0; i < cnt; i++)
			if (_bits[i].rgbReserved) {
				_bits[i].rgbRed	 = r;
				_bits[i].rgbGreen	 = g;
				_bits[i].rgbBlue	 = b;
				PreMulRGBA(_bits[i]);
			}
	}
}

RGBQUAD CTxDib::GetAreaColorInterpolated(float const xc, float const yc, float const w, float const h)
{
	RGBQUAD color; // calculated colour

	// area is wider and/or taller than one pixel:
	CTxDibRect2 area(xc-w/2.0f, yc-h/2.0f, xc+w/2.0f, yc+h/2.0f); // area
	int xi1 = (int)(area.botLeft.x+0.49999999f); // low x
	int yi1 = (int)(area.botLeft.y+0.49999999f); // low y

	int xi2 = (int)(area.topRight.x+0.5f); // top x
	int yi2 = (int)(area.topRight.y+0.5f); // top y (for loops)

	float rr, gg, bb, aa; // red, green, blue and alpha components
	rr = gg = bb = aa = 0;
	int x, y; // loop counters
	float s = 0; // surface of all pixels
	float cps; // surface of current crosssection
	if (h > 1 && w > 1) {
		// width and height of area are greater than one pixel, so we can employ "ordinary" averaging
		CTxDibRect2 intBL, intTR; // bottom left and top right intersection
		intBL = area.CrossSection(CTxDibRect2(((float)xi1)-0.5f, ((float)yi1)-0.5f, ((float)xi1)+0.5f, ((float)yi1)+0.5f));
		intTR = area.CrossSection(CTxDibRect2(((float)xi2)-0.5f, ((float)yi2)-0.5f, ((float)xi2)+0.5f, ((float)yi2)+0.5f));
		float wBL, wTR, hBL, hTR;
		wBL=intBL.Width(); // width of bottom left pixel-area intersection
		hBL=intBL.Height(); // height of bottom left...
		wTR=intTR.Width(); // width of top right...
		hTR=intTR.Height(); // height of top right...

		AddAveragingCont(GetPixelColorWithOverflow(xi1,yi1), wBL*hBL, rr, gg, bb, aa); // bottom left pixel
		AddAveragingCont(GetPixelColorWithOverflow(xi2,yi1), wTR*hBL, rr, gg, bb, aa); // bottom right pixel
		AddAveragingCont(GetPixelColorWithOverflow(xi1,yi2), wBL*hTR, rr, gg, bb, aa); // top left pixel
		AddAveragingCont(GetPixelColorWithOverflow(xi2,yi2), wTR*hTR, rr, gg, bb, aa); // top right pixel
		// bottom and top row
		for (x = xi1+1; x < xi2; x++) {
			AddAveragingCont(GetPixelColorWithOverflow(x,yi1), hBL, rr, gg, bb, aa); // bottom row
			AddAveragingCont(GetPixelColorWithOverflow(x,yi2), hTR, rr, gg, bb, aa); // top row
		}
		// leftmost and rightmost column
		for (y = yi1+1; y < yi2; y++) {
			AddAveragingCont(GetPixelColorWithOverflow(xi1,y), wBL, rr, gg, bb, aa); // left column
			AddAveragingCont(GetPixelColorWithOverflow(xi2,y), wTR, rr, gg, bb, aa); // right column
		}
		for (y = yi1+1; y < yi2; y++) {
			for (x = xi1+1; x < xi2; x++) {
				color=GetPixelColorWithOverflow(x,y);
				rr+=color.rgbRed;
				gg+=color.rgbGreen;
				bb+=color.rgbBlue;
				aa+=color.rgbReserved;
			}
		}
	}
	else {
		// width or height greater than one:
		CTxDibRect2 intersect; // intersection with current pixel
		CTxDibPoint2 center;
		for (y = yi1; y <= yi2; y++) 
			for (x = xi1; x <= xi2; x++)  {
				intersect = area.CrossSection(CTxDibRect2(((float)x)-0.5f, ((float)y)-0.5f, ((float)x)+0.5f, ((float)y)+0.5f));
				center = intersect.Center();
				color = GetPixelColorInterpolated(center.x, center.y);
				cps = intersect.Surface();
				rr += color.rgbRed*cps;
				gg += color.rgbGreen*cps;
				bb += color.rgbBlue*cps;
				aa += color.rgbReserved*cps;
			}
	}

	s = area.Surface();
	rr /= s; gg /= s; bb /= s; aa /= s;
	if (rr > 255) rr = 255; if (rr < 0) rr = 0; color.rgbRed = (BYTE)rr;
	if (gg > 255) gg = 255; if (gg < 0) gg = 0; color.rgbGreen = (BYTE)gg;
	if (bb > 255) bb = 255; if (bb < 0) bb = 0; color.rgbBlue = (BYTE)bb;
	if (aa > 255) aa = 255; if (aa < 0) aa = 0; color.rgbReserved = (BYTE)aa;
	return color;
}

typedef long fixed;												// Our new fixed point type
#define itofx(x) ((x) << 8)										// Integer to fixed point
#define ftofx(x) (long)((x) * 256)								// Float to fixed point
#define dtofx(x) (long)((x) * 256)								// Double to fixed point
#define fxtoi(x) ((x) >> 8)										// Fixed point to integer
#define fxtof(x) ((float) (x) / 256)							// Fixed point to float
#define fxtod(x) ((double)(x) / 256)							// Fixed point to double
#define Mulfx(x,y) (((x) * (y)) >> 8)							// Multiply a fixed by a fixed
#define Divfx(x,y) (((x) << 8) / (y))							// Divide a fixed by a fixed
#define _PIXEL DWORD											// Pixel

void CTxDib::Resample2(int newWidth, int newHeight, CTxDib *dst /*= NULL */)
{
	// Check for valid bitmap
	if (IsValid()) {
		// Calculate scaling params
		long _width = max(1, newWidth);
		long _height = max(1, newHeight);
		float dx = (float)_width  / (float)_width;
		float dy = (float)_height / (float)_height;
		fixed f_dx = ftofx(dx);
		fixed f_dy = ftofx(dy);
		fixed f_1 = itofx(1);

		LPRGBQUAD lpData = (LPRGBQUAD)malloc(_width * _height * sizeof(RGBQUAD));
		if (lpData) {
			// Scale bitmap
			DWORD dwDstHorizontalOffset;
			DWORD dwDstVerticalOffset = 0;
			DWORD dwDstTotalOffset;
			LPRGBQUAD lpSrcData = _bits;
			DWORD dwSrcTotalOffset;
			LPRGBQUAD lpDstData = lpData;
			for (long i = 0; i < _height; i++) {
				dwDstHorizontalOffset = 0;
				for (long j = 0; j < _width; j++) {
					// Update destination total offset
					dwDstTotalOffset = dwDstVerticalOffset + dwDstHorizontalOffset;

					// Update bitmap
					fixed f_i = itofx(i);
					fixed f_j = itofx(j);
					fixed f_a = Mulfx(f_i, f_dy);
					fixed f_b = Mulfx(f_j, f_dx);
					long m = fxtoi(f_a);
					long n = fxtoi(f_b);
					fixed f_f = f_a - itofx(m);
					fixed f_g = f_b - itofx(n);
					dwSrcTotalOffset = m * _width + n;
					DWORD dwSrcTopLeft = dwSrcTotalOffset;
					DWORD dwSrcTopRight = dwSrcTotalOffset + 1;
					if (n >= _width-1)
						dwSrcTopRight = dwSrcTotalOffset;
					DWORD dwSrcBottomLeft = dwSrcTotalOffset + _width;
					if (m >= _height-1)
						dwSrcBottomLeft = dwSrcTotalOffset;
					DWORD dwSrcBottomRight = dwSrcTotalOffset + _width + 1;
					if (n >= _width-1 || m >= _height-1)
						dwSrcBottomRight = dwSrcTotalOffset;
					fixed f_w1 = Mulfx(f_1-f_f, f_1-f_g);
					fixed f_w2 = Mulfx(f_1-f_f, f_g);
					fixed f_w3 = Mulfx(f_f, f_1-f_g);
					fixed f_w4 = Mulfx(f_f, f_g);
					RGBQUAD pixel1 = lpSrcData[dwSrcTopLeft];
					RGBQUAD pixel2 = lpSrcData[dwSrcTopRight];
					RGBQUAD pixel3 = lpSrcData[dwSrcBottomLeft];
					RGBQUAD pixel4 = lpSrcData[dwSrcBottomRight];
					fixed f_r1 = itofx(pixel1.rgbRed);
					fixed f_r2 = itofx(pixel2.rgbRed);
					fixed f_r3 = itofx(pixel3.rgbRed);
					fixed f_r4 = itofx(pixel4.rgbRed);
					fixed f_g1 = itofx(pixel1.rgbGreen);
					fixed f_g2 = itofx(pixel2.rgbGreen);
					fixed f_g3 = itofx(pixel3.rgbGreen);
					fixed f_g4 = itofx(pixel4.rgbGreen);
					fixed f_b1 = itofx(pixel1.rgbBlue);
					fixed f_b2 = itofx(pixel2.rgbBlue);
					fixed f_b3 = itofx(pixel3.rgbBlue);
					fixed f_b4 = itofx(pixel4.rgbBlue);
					fixed f_a1 = itofx(pixel1.rgbReserved);
					fixed f_a2 = itofx(pixel2.rgbReserved);
					fixed f_a3 = itofx(pixel3.rgbReserved);
					fixed f_a4 = itofx(pixel4.rgbReserved);
					lpDstData[dwDstTotalOffset].rgbRed = (BYTE)fxtoi(Mulfx(f_w1, f_r1) + Mulfx(f_w2, f_r2) + Mulfx(f_w3, f_r3) + Mulfx(f_w4, f_r4));
					lpDstData[dwDstTotalOffset].rgbGreen = (BYTE)fxtoi(Mulfx(f_w1, f_g1) + Mulfx(f_w2, f_g2) + Mulfx(f_w3, f_g3) + Mulfx(f_w4, f_g4));
					lpDstData[dwDstTotalOffset].rgbBlue = (BYTE)fxtoi(Mulfx(f_w1, f_b1) + Mulfx(f_w2, f_b2) + Mulfx(f_w3, f_b3) + Mulfx(f_w4, f_b4));
					lpDstData[dwDstTotalOffset].rgbReserved = (BYTE)fxtoi(Mulfx(f_w1, f_a1) + Mulfx(f_w2, f_a2) + Mulfx(f_w3, f_a3) + Mulfx(f_w4, f_a4));

					// Update destination horizontal offset
					dwDstHorizontalOffset ++;
				}
				dwDstVerticalOffset += _width;
			}

			if (dst) {
				dst->_copy(lpData, newWidth, newHeight);
				dst->_maxAlpha = _maxAlpha;
			}
			else
				_copy(lpData, newWidth, newHeight);
		}
	}
}

bool CTxDib::QIShrink(int newWidth, int newHeight, CTxDib * dst /*= NULL */)
{
	if (!IsValid()) return false;
	if (newWidth > _width || newHeight > _height) 
		return false;
	if (newWidth == _width && newHeight == _height)  {
		if (dst)
			*dst = *this;
		return true;
	}

	LPRGBQUAD newBits = (LPRGBQUAD)malloc(newWidth * newHeight * sizeof(RGBQUAD));
	const int oldx = _width;
	const int oldy = _height;

	int accuCellSize = 5;

	unsigned int *accu = new unsigned int[newWidth * accuCellSize]; // array for summing pixels... one pixel for every destination column
	unsigned int *accuPtr; // pointer for walking through accu
	// each cell consists of blue, red, green component and count of pixels summed in this cell
	memset(accu, 0, newWidth * accuCellSize * sizeof(unsigned int)); // clear accu

	// RGB24 version with pointers
	LPRGBQUAD destPtr, srcPtr, destPtrS, srcPtrS;        //destination and source pixel, and beginnings of current row
	srcPtrS = _bits;
	destPtrS = newBits;
	int ex = 0, ey = 0; // ex and ey replace division... 
	int dy = 0;
	// (we just add pixels, until by adding newWidth or newHeight we get a number greater than old size... then it's time to move to next pixel)
	for (int y = 0; y < oldy; y++) { // for all source rows
		ey += newHeight;
		ex = 0; // restart with ex = 0
		accuPtr = accu; // restart from beginning of accu
		srcPtr = srcPtrS; // and from new source line
		for (int x = 0; x < oldx; x++) { // for all source columns
			ex += newWidth;
			accuPtr[0] += srcPtr[x].rgbRed; // add current pixel to current accu slot
			accuPtr[1] += srcPtr[x].rgbGreen;
			accuPtr[2] += srcPtr[x].rgbBlue;
			accuPtr[4] += srcPtr[x].rgbReserved;
			accuPtr[3]++;
			if (ex > oldx) { // when we reach oldx, it's time to move to new slot
				accuPtr += accuCellSize;
				ex -= oldx; // (substract oldx from ex and resume from there on)
			}
		}

		if (ey >= oldy) { // now when this happens
			ey -= oldy; // it's time to move to new destination row
			destPtr = destPtrS; // reset pointers to proper initial values
			accuPtr = accu;
			for (int k = 0; k < newWidth; k++) { // copy accu to destination row (divided by number of pixels in each slot)
				destPtr[k].rgbRed = (BYTE)(accuPtr[0] / accuPtr[3]);
				destPtr[k].rgbGreen = (BYTE)(accuPtr[1] / accuPtr[3]);
				destPtr[k].rgbBlue = (BYTE)(accuPtr[2] / accuPtr[3]);
				destPtr[k].rgbReserved = (BYTE)(accuPtr[4] / accuPtr[3]);
				accuPtr += accuCellSize;
			}
			memset(accu, 0, newWidth * accuCellSize * sizeof(unsigned int)); // clear accu
			destPtrS += newWidth;
		}

		srcPtrS += _width; // next round we start from new source row
	}
	delete [] accu; // delete helper array

	// copy new image to the destination
	if (dst) {
		dst->_copy(newBits, newWidth, newHeight);
		dst->_maxAlpha = _maxAlpha;
	}
	else
		_copy(newBits, newWidth, newHeight);
	return true;
}

BOOL CTxDib::SavePNG(LPCWSTR fileName)
{
	FIBITMAP *dib = FreeImage_Allocate(_width, _height, 32);
	memcpy(FreeImage_GetBits(dib), _bits, _width * _height * 4);
	BOOL ret = FreeImage_SaveU(FIF_PNG, dib, fileName);
	FreeImage_Unload(dib);
	return ret;
}

BOOL CTxDib::SaveJPG(LPCWSTR fileName, int quality /*= JPEG_QUALITY_GOOD*/)
{
	FIBITMAP *dib = FreeImage_Allocate(_width, _height, 32);
	memcpy(FreeImage_GetBits(dib), _bits, _width * _height * 4);
	FIBITMAP *dib24 = FreeImage_ConvertTo24Bits(dib);
	int flags = JPEG_QUALITYGOOD;
	switch (quality) {
	case JPEG_QUALITY_SUPER: flags = JPEG_QUALITYSUPERB; break;
	case JPEG_QUALITY_GOOD: flags = JPEG_QUALITYGOOD; break;
	case JPEG_QUALITY_NORMAL: flags = JPEG_QUALITYNORMAL; break;
	case JPEG_QUALITY_AVERAGE: flags = JPEG_QUALITYAVERAGE; break;
	case JPEG_QUALITY_BAD: flags = JPEG_QUALITYBAD; break;
	}
	BOOL ret = FreeImage_SaveU(FIF_JPEG, dib24, fileName, flags);
	FreeImage_Unload(dib);
	FreeImage_Unload(dib24);
	return ret;
}

BOOL CTxDib::SaveBMP(LPCWSTR fileName)
{
	FIBITMAP *dib = FreeImage_Allocate(_width, _height, 32);
	memcpy(FreeImage_GetBits(dib), _bits, _width * _height * 4);
	BOOL ret = FreeImage_SaveU(FIF_BMP, dib, fileName, BMP_SAVE_RLE);
	FreeImage_Unload(dib);
	return ret;
}

BOOL CTxDib::CalcAlpha(CTxDib *imgWhite, CTxDib *imgBlack)
{
	if (!imgBlack || !imgWhite)	
		return FALSE;
	if (imgWhite->GetWidth()  != imgBlack->GetWidth() ||
		imgWhite->GetHeight() != imgBlack->GetHeight())
		return FALSE;
	Destroy();

	_width = imgBlack->GetWidth();
	_height	= imgBlack->GetHeight();
	size_t cnt = _width * _height;
	size_t sz = cnt * sizeof(RGBQUAD);
	_bits = (LPRGBQUAD) malloc(sz);

	int alphaR, alphaG, alphaB, resultR, resultG, resultB;
	for (size_t i=0; i < cnt; i++) {
		alphaR = imgBlack->_bits[i].rgbRed - imgWhite->_bits[i].rgbRed + 255;
		alphaG = imgBlack->_bits[i].rgbGreen - imgWhite->_bits[i].rgbGreen + 255;
		alphaB = imgBlack->_bits[i].rgbBlue - imgWhite->_bits[i].rgbBlue + 255;
		if (alphaG != 0) {
			resultR = imgBlack->_bits[i].rgbRed * 255 / alphaG;
			resultG = imgBlack->_bits[i].rgbGreen * 255 / alphaG;
			resultB = imgBlack->_bits[i].rgbBlue * 255 / alphaG;
		}
		else {
			resultR = 0;
			resultG = 0;
			resultB = 0;
		}
		_bits[i].rgbReserved = (BYTE)alphaG;
		_bits[i].rgbRed = (BYTE)resultR;
		_bits[i].rgbGreen = (BYTE)resultG;
		_bits[i].rgbBlue = (BYTE)resultB;
		PreMulRGBA(_bits[i]);
	}
	return TRUE;
}

#define RBLOCK 96

void CTxDib::RotateLeft(CTxDib *dst /*= NULL*/)
{
	if (!IsValid()) return;

	int width = GetHeight();
	int height = GetWidth();
	size_t sz = width * height * sizeof(RGBQUAD);
	LPRGBQUAD newBbits = (LPRGBQUAD)malloc(sz);

	int xs, ys; // x-segment and y-segment
	long x, x2, y;
	LPRGBQUAD srcPtr;
	LPRGBQUAD dstPtr;
	for (xs = 0; xs < width; xs += RBLOCK) { // for all image blocks of RBLOCK*RBLOCK pixels
		for (ys = 0; ys < height; ys += RBLOCK) // RGB24 optimized pixel access:
			for (x = xs; x < min(width, xs + RBLOCK); x++) { // do rotation
				x2 = width - x - 1;
				dstPtr = newBbits + ys * width + x;
				srcPtr = _bits + x2 * _width + ys;
				for (y = ys; y < min(height, ys + RBLOCK); y++) {
					*dstPtr = *srcPtr;
					srcPtr++;
					dstPtr+= width;
				}
			}
	}
	if (dst)
		dst->_copy(newBbits, width, height, FALSE);
	else
		_copy(newBbits, width, height, FALSE);
}

void CTxDib::RotateRight(CTxDib *dst /*= NULL*/)
{
	if (!IsValid()) return;

	int width = GetHeight();
	int height = GetWidth();
	size_t sz = width * height * sizeof(RGBQUAD);
	LPRGBQUAD newBbits = (LPRGBQUAD)malloc(sz);

	int xs, ys; // x-segment and y-segment
	long x, y2, y;
	LPRGBQUAD srcPtr;
	LPRGBQUAD dstPtr;
	for (xs = 0; xs < width; xs += RBLOCK) { // for all image blocks of RBLOCK*RBLOCK pixels
		for (ys = 0; ys < height; ys += RBLOCK) // RGB24 optimized pixel access:
			for (y = ys; y < min(height, ys + RBLOCK); y++) { // do rotation
				y2 = height - y - 1;
				dstPtr = newBbits + y * width + xs;
				srcPtr = _bits + xs * _width + y2;
				for (x = xs; x < min(width, xs + RBLOCK); x++) {
					*dstPtr = *srcPtr;
					dstPtr++;
					srcPtr += _width;
				}
			}
	}
	if (dst)
		dst->_copy(newBbits, width, height, FALSE);
	else
		_copy(newBbits, width, height, FALSE);
}

//////////////////////////////////////////////////////////////////////////

CTxSkinDIB::CTxSkinDIB()
{
	ZeroMemory(&_margins, sizeof(_margins));
	_tileX	= FALSE;
	_tileY	= FALSE;
}

CTxSkinDIB::~CTxSkinDIB()
{
}

BOOL CTxSkinDIB::Load(LPCWSTR fileName, MARGINS *mg, BOOL tileX, BOOL tileY)
{
	CTxDib dib;
	if (dib.Load(fileName))
		return Load(&dib, mg, tileX, tileY);
	return FALSE;
}

BOOL CTxSkinDIB::Load(CTxDib *dib, MARGINS *mg, BOOL tileX, BOOL tileY)
{
	if (!dib) return FALSE;
	_margins = *mg;
	_tileX = tileX;
	_tileY = tileY;
	if (_margins.cxLeftWidth) {
		if (_margins.cyTopHeight)
			dib->Crop(0, 0, _margins.cxLeftWidth, _margins.cyTopHeight, &_dibLeftTop);
		if (_margins.cyBottomHeight)
			dib->Crop(0, dib->GetHeight() - _margins.cyBottomHeight, _margins.cxLeftWidth, dib->GetHeight(), &_dibLeftBottom);
		dib->Crop(0, _margins.cyTopHeight, _margins.cxLeftWidth, dib->GetHeight() - _margins.cyBottomHeight, &_dibLeftCenter);
	}
	if (_margins.cxRightWidth) {
		if (_margins.cyTopHeight)
			dib->Crop(dib->GetWidth() - _margins.cxRightWidth, 0, dib->GetWidth(), _margins.cyTopHeight, &_dibRightTop);
		if (_margins.cyBottomHeight)
			dib->Crop(dib->GetWidth() - _margins.cxRightWidth, dib->GetHeight() - _margins.cyBottomHeight, dib->GetWidth(), dib->GetHeight(), &_dibRightBottom);
		dib->Crop(dib->GetWidth() - _margins.cxRightWidth, _margins.cyTopHeight, dib->GetWidth(), dib->GetHeight() - _margins.cyBottomHeight, &_dibRightCenter);
	}
	if (_margins.cyTopHeight)
		dib->Crop(_margins.cxLeftWidth, 0, dib->GetWidth() - _margins.cxRightWidth, _margins.cyTopHeight, &_dibTop);
	if (_margins.cyBottomHeight)
		dib->Crop(_margins.cxLeftWidth, dib->GetHeight() - _margins.cyBottomHeight, dib->GetWidth() - _margins.cxRightWidth, dib->GetHeight(), &_dibBottom);
	dib->Crop(_margins.cxLeftWidth, _margins.cyTopHeight, dib->GetWidth() - _margins.cxRightWidth, dib->GetHeight() - _margins.cyBottomHeight, &_dibCenter);
	return TRUE;
}

void CTxSkinDIB::Draw(HDC hdc, LPRECT rcDraw, LPRECT rcClip)
{
	RECT rcPart;
	RECT rcTmp;
	if (_margins.cxLeftWidth) {
		if (_margins.cyTopHeight) {
			rcPart.left = rcDraw->left;
			rcPart.right = rcPart.left + _margins.cxLeftWidth;
			rcPart.top = rcDraw->top;
			rcPart.bottom = rcPart.top + _margins.cyTopHeight;
			if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
				_dibLeftTop.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
		}
		if (_margins.cyBottomHeight) {
			rcPart.left = rcDraw->left;
			rcPart.right = rcPart.left + _margins.cxLeftWidth;
			rcPart.top = rcDraw->bottom - _margins.cyBottomHeight;
			rcPart.bottom = rcPart.top + _margins.cyBottomHeight;
			if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
				_dibLeftBottom.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
		}
		rcPart.left = rcDraw->left;
		rcPart.right = rcPart.left + _margins.cxLeftWidth;
		rcPart.top = rcDraw->top + _margins.cyTopHeight;
		rcPart.bottom = rcDraw->bottom - _margins.cyBottomHeight;
		if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart)) {
			if (!_tileY)
				_dibLeftCenter.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			else
				_dibLeftCenter.Tile(hdc, &rcPart, rcClip);
		}
	}

	if (_margins.cxRightWidth) {
		if (_margins.cyTopHeight) {
			rcPart.left = rcDraw->right - _margins.cxRightWidth;
			rcPart.right = rcPart.left + _margins.cxRightWidth;
			rcPart.top = rcDraw->top;
			rcPart.bottom = rcPart.top + _margins.cyTopHeight;
			if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
				_dibRightTop.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
		}
		if (_margins.cyBottomHeight) {
			rcPart.left = rcDraw->right - _margins.cxRightWidth;
			rcPart.right = rcPart.left + _margins.cxRightWidth;
			rcPart.top = rcDraw->bottom - _margins.cyBottomHeight;
			rcPart.bottom = rcPart.top + _margins.cyBottomHeight;
			if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
				_dibRightBottom.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
		}
		rcPart.left = rcDraw->right - _margins.cxRightWidth;
		rcPart.right = rcPart.left + _margins.cxRightWidth;
		rcPart.top = rcDraw->top + _margins.cyTopHeight;
		rcPart.bottom = rcDraw->bottom - _margins.cyBottomHeight;
		if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart)) {
			if (!_tileY)
				_dibRightCenter.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			else
				_dibRightCenter.Tile(hdc, &rcPart, rcClip);
		}
	}

	if (_margins.cyTopHeight) {
		rcPart.left = rcDraw->left + _margins.cxLeftWidth;
		rcPart.right = rcDraw->right - _margins.cxRightWidth;
		rcPart.top = rcDraw->top;
		rcPart.bottom = rcDraw->top + _margins.cyTopHeight;
		if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart)) {
			if (!_tileX)
				_dibTop.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			else
				_dibTop.Tile(hdc, &rcPart, rcClip);
		}
	}

	if (_margins.cyBottomHeight) {
		rcPart.left = rcDraw->left + _margins.cxLeftWidth;
		rcPart.right = rcDraw->right - _margins.cxRightWidth;
		rcPart.top = rcDraw->bottom - _margins.cyBottomHeight;
		rcPart.bottom = rcPart.top + _margins.cyBottomHeight;
		if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart)) {
			if (!_tileX)
				_dibBottom.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			else
				_dibBottom.Tile(hdc, &rcPart, rcClip);
		}
	}

	rcPart.left = rcDraw->left + _margins.cxLeftWidth;
	rcPart.right = rcDraw->right - _margins.cxRightWidth;
	rcPart.top = rcDraw->top + _margins.cyTopHeight;
	rcPart.bottom = rcDraw->bottom - _margins.cyBottomHeight;
	if (!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart)) {
		if (!_tileY && !_tileX)
			_dibCenter.Draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
		else if (_tileX && _tileY)
			_dibCenter.Tile(hdc, &rcPart, rcClip);
		else if (_tileX && !_tileY) {
			CTxDib dib;
			_dibCenter.Resample(_dibCenter.GetWidth(), rcPart.bottom - rcPart.top, &dib);
			dib.Tile(hdc, &rcPart, rcClip);
		}
		else {
			CTxDib dib;
			_dibCenter.Resample(rcPart.right - rcPart.left, _dibCenter.GetHeight(), &dib);
			dib.Tile(hdc, &rcPart, rcClip);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CTxDibSet::CTxDibSet(CTxDib *img, int rows, int cols)
{
	_cols = cols;
	_rows = rows;
	_width = img->GetWidth() / cols;
	_height = img->GetHeight() / rows;
	for (int row = 0; row < rows; row++) {
		imgCols vCols;
		for (int col = 0; col < cols; col++) {
			CTxDib *frame = new CTxDib;
			img->Crop(col * _width, row * _height, (col + 1) * _width, (row + 1) * _height, frame);
			vCols.push_back(frame);
		}
		_items.push_back(vCols);
	}
}

CTxDibSet::~CTxDibSet()
{
	for (size_t row = 0; row < _items.size(); row++)
		for (size_t col = 0; col < _items[row].size(); col++)
			delete _items[row][col];
}

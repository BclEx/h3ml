#include <Dib.h>

Dib::Dib()
{
	_bmp = NULL;
	_oldBmp = NULL;
	_hdc = NULL;
	_bits = NULL;
	_ownData = FALSE;
	_width = 0;
	_height = 0;
	_hTargetDC = NULL;
	_restoreViewPort = FALSE;
}

Dib::~Dib()
{
	Destroy();
}

void Dib::Destroy(bool delBmp)
{
	if (_hdc && _ownData) {
		SelectObject(_hdc, _oldBmp);
		if (delBmp)
			DeleteObject(_bmp);
		DeleteDC(_hdc);
	}
	else if (_restoreViewPort && _hdc)
		SetWindowOrgEx(_hdc, _oldViewPort.x, _oldViewPort.y, NULL);
	_bmp = NULL;
	_oldBmp = NULL;
	_hdc = NULL;
	_bits = NULL;
	_ownData = FALSE;
	_width = 0;
	_height = 0;
}

bool Dib::Create(int width, int height, bool topdowndib /*= false*/)
{
	Destroy();
	BITMAPINFO bmpInfo;
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = width;
	bmpInfo.bmiHeader.biHeight = height * (topdowndib ? -1 : 1);
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biSizeImage = 0;
	bmpInfo.bmiHeader.biXPelsPerMeter = 0;
	bmpInfo.bmiHeader.biYPelsPerMeter = 0;
	bmpInfo.bmiHeader.biClrUsed = 0;
	bmpInfo.bmiHeader.biClrImportant = 0;
	_hdc = CreateCompatibleDC(NULL);
	_bmp = ::CreateDIBSection(_hdc, &bmpInfo, DIB_RGB_COLORS, (LPVOID *)&_bits, 0, 0);
	if (_bits)
		_oldBmp = (HBITMAP)::SelectObject(_hdc, _bmp);
	else {
		DeleteDC(_hdc);
		_hdc = NULL;
	}
	if (_hdc) {
		_width = width;
		_height = height;
		_ownData = TRUE;
		return true;
	}
	return false;
}

/*
bool Dib::create(HDC hdc, HBITMAP bmp, LPRGBQUAD bits, int width, int height, int shift_x, int shift_y)
{
Destroy();
_bmp = bmp;
_hdc = hdc;
_bits = bits;
_width = width;
_height = height;
_ownData = FALSE;
SetWindowOrgEx(_hdc, -shift_x, -shift_y, &m_oldViewPort);
_restoreViewPort = TRUE;
return true;
}

*/
bool Dib::Create(HDC hdc, HBITMAP bmp, LPRGBQUAD bits, int width, int height)
{
	Destroy();
	_bmp = bmp;
	_hdc = hdc;
	_bits = bits;
	_width = width;
	_height = height;
	_ownData = FALSE;
	_restoreViewPort = FALSE;
	return true;
}

void Dib::Clear()
{
	if (_bits)
		ZeroMemory(_bits, _width * _height * 4);
}

void Dib::Draw(HDC hdc, int x, int y)
{
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = 255;
	AlphaBlend(hdc, x, y, _width, _height, _hdc, 0, 0, _width, _height, bf);
}

void Dib::Draw(HDC hdc, LPRECT rcDraw)
{
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = 255;
	AlphaBlend(hdc,
		rcDraw->left, rcDraw->top,
		rcDraw->right - rcDraw->left,
		rcDraw->bottom - rcDraw->top, _hdc,
		rcDraw->left, rcDraw->top,
		rcDraw->right - rcDraw->left,
		rcDraw->bottom - rcDraw->top,
		bf);
}

HDC Dib::BeginPaint(HDC hdc, LPRECT rcDraw)
{
	if (Create(rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top, true)) {
		_hTargetDC = hdc;
		_rcTarget = *rcDraw;
		SetWindowOrgEx(_hdc, rcDraw->left, rcDraw->top, &_oldViewPort);
		return _hdc;
	}
	return NULL;
}

void Dib::EndPaint(bool copy)
{
	BOOL draw = TRUE;
	SetWindowOrgEx(_hdc, _oldViewPort.x, _oldViewPort.y, NULL);
	if (!copy) {
		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.AlphaFormat = AC_SRC_ALPHA;
		bf.SourceConstantAlpha = 255;
		AlphaBlend(_hTargetDC, _rcTarget.left, _rcTarget.top,
			_rcTarget.right - _rcTarget.left,
			_rcTarget.bottom - _rcTarget.top, _hdc,
			0, 0,
			_rcTarget.right - _rcTarget.left,
			_rcTarget.bottom - _rcTarget.top,
			bf);
	}
	else {
		BitBlt(_hTargetDC, _rcTarget.left, _rcTarget.top,
			_rcTarget.right - _rcTarget.left,
			_rcTarget.bottom - _rcTarget.top, _hdc, 0, 0, SRCCOPY);
	}
	_hTargetDC = NULL;
	Destroy();
}

HBITMAP Dib::DetachBitmap()
{
	HBITMAP bmp = _bmp;
	Destroy(false);
	return bmp;
}

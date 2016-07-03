#pragma once

#define ISOLATION_AWARE_ENABLED		1
#define GDIPVER	0x0110

#include "targetver.h"
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <litehtml.h>
#include <winhttp.h>
#include <algorithm>
#include <TxDib.h>
#include <Dib.h>

//
// include CAIRO project
#include <cairo.h>
#include <cairo-win32.h>
#pragma comment(lib, "cairo.lib")
//
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Msimg32.lib")

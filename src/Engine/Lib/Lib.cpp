#include "..\Global.h"
#include <stdarg.h>

#if defined(MACOS_X)
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

int Lib::frameNumber = 0;
bool Lib::mainThreadInitialized = 0;
bool Lib::isMainThread = 0;
char Exception::error[2048];

void Lib::Init() {
	assert(sizeof(bool) == 1);
	isMainThread = 1;
	mainThreadInitialized = 1;	// note that the thread-local isMainThread is now valid
}

void Lib::ShutDown() {
}

#pragma region Colors

vec4 colorBlack = vec4(0.00f, 0.00f, 0.00f, 1.00f);
vec4 colorWhite = vec4(1.00f, 1.00f, 1.00f, 1.00f);
vec4 colorRed = vec4(1.00f, 0.00f, 0.00f, 1.00f);
vec4 colorGreen = vec4(0.00f, 1.00f, 0.00f, 1.00f);
vec4 colorBlue = vec4(0.00f, 0.00f, 1.00f, 1.00f);
vec4 colorYellow = vec4(1.00f, 1.00f, 0.00f, 1.00f);
vec4 colorMagenta = vec4(1.00f, 0.00f, 1.00f, 1.00f);
vec4 colorCyan = vec4(0.00f, 1.00f, 1.00f, 1.00f);
vec4 colorOrange = vec4(1.00f, 0.50f, 0.00f, 1.00f);
vec4 colorPurple = vec4(0.60f, 0.00f, 0.60f, 1.00f);
vec4 colorPink = vec4(0.73f, 0.40f, 0.48f, 1.00f);
vec4 colorBrown = vec4(0.40f, 0.35f, 0.08f, 1.00f);
vec4 colorLtGrey = vec4(0.75f, 0.75f, 0.75f, 1.00f);
vec4 colorMdGrey = vec4(0.50f, 0.50f, 0.50f, 1.00f);
vec4 colorDkGrey = vec4(0.25f, 0.25f, 0.25f, 1.00f);

#pragma endregion

void Lib::FatalError(const char *fmt, ...) {
	va_list argptr;
	char text[MAX_STRING_CHARS];
	va_start(argptr, fmt);
	_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);
	//common->FatalError("%s", text);
	printf("%s", text);
}

void Lib::Error(const char *fmt, ...) {
	va_list argptr;
	char text[MAX_STRING_CHARS];
	va_start(argptr, fmt);
	_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);
	//common->Error("%s", text);
	printf("%s", text);
}

void Lib::Warning(const char *fmt, ...) {
	va_list argptr;
	char text[MAX_STRING_CHARS];
	va_start(argptr, fmt);
	_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);
	//common->Warning("%s", text);
	printf("%s", text);
}

void Lib::WarningIf(const bool test, const char *fmt, ...) {
	if (!test)
		return;
	va_list argptr;
	char text[MAX_STRING_CHARS];
	va_start(argptr, fmt);
	_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);
	//common->Warning("%s", text);
	printf("%s", text);
}

void Lib::Printf(const char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	//if (common)
	//	common->VPrintf(fmt, argptr);
	vprintf(fmt, argptr);
	va_end(argptr);
}

void Lib::PrintfIf(const bool test, const char *fmt, ...) {
	if (!test)
		return;
	va_list argptr;
	va_start(argptr, fmt);
	//common->VPrintf(fmt, argptr);
	vprintf(fmt, argptr);
	va_end(argptr);
}
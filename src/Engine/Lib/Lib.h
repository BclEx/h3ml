#ifndef __LIB_H__
#define __LIB_H__

#include <stddef.h>

class Lib {
private:
	static bool mainThreadInitialized;
	static bool isMainThread;

public:
	//static class idSys *sys;
	//static class idCommon *common;
	//static class idCVarSystem *cvarSystem;
	//static class idFileSystem *fileSystem;
	static int frameNumber;

	static void Init();
	static void ShutDown();

	// wrapper to idCommon functions 
	static void Printf(const char *fmt, ...);
	static void PrintfIf(const bool test, const char *fmt, ...);
	static void Error(const char *fmt, ...);
	static void FatalError(const char *fmt, ...);
	static void Warning(const char *fmt, ...);
	static void WarningIf(const bool test, const char *fmt, ...);

	// the extra check for mainThreadInitialized is necessary for this to be accurate
	// when called by startup code that happens before idLib::Init
	static bool IsMainThread() { return (0 == mainThreadInitialized) || (1 == isMainThread); }
};

//typedef int qhandle_t;
//class idFile;
//class idVec3;
//class idVec4;

#ifndef NULL
#define NULL					((void *)0)
#endif

#ifndef BIT
#define BIT(num)				(1ULL << (num))
#endif

#define	MAX_STRING_CHARS		1024		// max length of a string
#define MAX_PRINT_MSG			16384		// buffer size for our various printf routines

// maximum world size
#define MAX_WORLD_COORD			(128 * 1024)
#define MIN_WORLD_COORD			(-128 * 1024)
#define MAX_WORLD_SIZE			(MAX_WORLD_COORD - MIN_WORLD_COORD)

#define SIZE_KB(x)						(((x) + 1023) / 1024)
#define SIZE_MB(x)						(((SIZE_KB(x)) + 1023) / 1024)
#define SIZE_GB(x)						(((SIZE_MB(x)) + 1023) / 1024)

// basic colors
extern vec4 colorBlack;
extern vec4 colorWhite;
extern vec4 colorRed;
extern vec4 colorGreen;
extern vec4 colorBlue;
extern vec4 colorYellow;
extern vec4 colorMagenta;
extern vec4 colorCyan;
extern vec4 colorOrange;
extern vec4 colorPurple;
extern vec4 colorPink;
extern vec4 colorBrown;
extern vec4 colorLtGrey;
extern vec4 colorMdGrey;
extern vec4 colorDkGrey;

class Exception {
public:
	static const int MAX_ERROR_LEN = 2048;
	Exception(const char *text = "") { strncpy(error, text, MAX_ERROR_LEN); }
	const char *GetError() { return error; }
protected:
	// if GetError() were correctly const this would be named GetError(), too
	char *GetErrorBuffer() { return error; }
	int GetErrorBufferSize() { return MAX_ERROR_LEN; }
private:
	friend class FatalException;
	static char error[MAX_ERROR_LEN];
};

class FatalException {
public:
	static const int MAX_ERROR_LEN = 2048;
	FatalException(const char *text = "") { strncpy(Exception::error, text, MAX_ERROR_LEN); }
	const char *GetError() { return Exception::error; }
protected:
	char *GetErrorBuffer() { return Exception::error; }
	int GetErrorBufferSize() { return MAX_ERROR_LEN; }
};

struct Event_t {
};

// text manipulation
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"

// misc
#include "Dict.h"

#endif	/* !__LIB_H__ */

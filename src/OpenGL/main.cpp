#include <GL/glew.h>
#include <gl/glu.h>
#include <stdio.h>
#include <string>
#include <cstdlib>
#if defined(_WIN32)
#include <Windows.h>
#elif defined(POSIX)
#include "unistd.h"
#endif

void ThreadSleep(unsigned long nMilliseconds)
{
#if defined(_WIN32)
	::Sleep(nMilliseconds);
#elif defined(POSIX)
	usleep(nMilliseconds * 1000);
#endif
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
	return 0;
}
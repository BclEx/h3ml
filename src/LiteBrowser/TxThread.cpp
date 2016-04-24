#include "globals.h"
#include "TxThread.h"

CTxThread::CTxThread()
{
	_hThread = NULL;
	_hStop = NULL;
	_trdID = NULL;
}

CTxThread::~CTxThread()
{
	Stop();
	if (_hThread) CloseHandle(_hThread);
	if (_hStop) CloseHandle(_hStop);
}

void CTxThread::Run()
{
	Stop();
	_hStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	_hThread = CreateThread(NULL, 0, sThreadProc, (LPVOID)this, 0, &_trdID);
}

void CTxThread::Stop()
{
	if (_hThread) {
		if (_hStop)
			SetEvent(_hStop);
		WaitForSingleObject(_hThread, INFINITE);
		CloseHandle(_hThread);
		_hThread = NULL;
	}
	if (_hStop) CloseHandle(_hStop);
	_hStop = NULL;
}

DWORD WINAPI CTxThread::sThreadProc(LPVOID lpParameter)
{
	CTxThread *this_ = (CTxThread *)lpParameter;
	return this_->ThreadProc();
}

BOOL CTxThread::WaitForStop(DWORD ms)
{
	return (WaitForSingleObject(_hStop, ms) != WAIT_TIMEOUT ? TRUE : FALSE);
}

void CTxThread::PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (_hThread)
		PostThreadMessage(_trdID, Msg, wParam, lParam);
}
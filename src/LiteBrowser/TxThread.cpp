#include "globals.h"
#include "TxThread.h"

TxThread::TxThread()
{
	_hThread = NULL;
	_hStop = NULL;
	_trdID = NULL;
}

TxThread::~TxThread()
{
	Stop();
	if (_hThread) CloseHandle(_hThread);
	if (_hStop) CloseHandle(_hStop);
}

void TxThread::Run()
{
	Stop();
	_hStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	_hThread = CreateThread(NULL, 0, sThreadProc, (LPVOID)this, 0, &_trdID);
}

void TxThread::Stop()
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

DWORD WINAPI TxThread::sThreadProc(LPVOID lpParameter)
{
	TxThread *this_ = (TxThread *)lpParameter;
	return this_->ThreadProc();
}

BOOL TxThread::WaitForStop(DWORD ms)
{
	return (WaitForSingleObject(_hStop, ms) != WAIT_TIMEOUT ? TRUE : FALSE);
}

void TxThread::PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (_hThread)
		PostThreadMessage(_trdID, Msg, wParam, lParam);
}
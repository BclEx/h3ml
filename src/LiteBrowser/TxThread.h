#pragma once

class TxThread
{
	HANDLE _hThread;
	HANDLE _hStop;
	DWORD _trdID;

	static DWORD WINAPI sThreadProc(LPVOID lpParameter);
public:
	TxThread(void);
	virtual ~TxThread(void);
	virtual DWORD ThreadProc() = 0;

	DWORD GetID() { return _trdID; }
	void Run();
	void Stop();
	BOOL WaitForStop(DWORD ms);
	void PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
};

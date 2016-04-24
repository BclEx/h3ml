#pragma once

#include <Windows.h>
#include <winhttp.h>
#include <stdlib.h>
#include <vector>

class HttpRequest
{
	friend class Http;
public:
	typedef std::vector<HttpRequest *> vector;
protected:
	HINTERNET _hConnection;
	HINTERNET _hRequest;
	CRITICAL_SECTION _sync;
	Http *_http;
	BYTE _buffer[8192];
	DWORD _error;
	ULONG64 _content_length;
	ULONG64 _downloaded_length;
	DWORD _status;
	std::wstring _url;
	LONG _refCount;
public:
	HttpRequest();
	virtual ~HttpRequest();

	virtual void OnFinish(DWORD dwError, LPCWSTR errMsg) = 0;
	virtual void OnData(LPCBYTE data, DWORD len, ULONG64 downloaded, ULONG64 total) = 0;
	virtual void OnHeadersReady(HINTERNET hRequest);

	BOOL Create(LPCWSTR url, HINTERNET hSession);
	void Cancel();
	void Lock();
	void Unlock();
	ULONG64	GetContentLength();
	DWORD GetStatusCode();
	void AddRef();
	void Release();

protected:
	DWORD OnSendRequestComplete();
	DWORD OnHeadersAvailable();
	DWORD OnHandleClosing();
	DWORD OnRequestError(DWORD dwError);
	DWORD OnReadComplete(DWORD len);
	DWORD ReadData();
	void SetParent(Http *parent);
};

class Http
{
	friend class HttpRequest;

	HINTERNET _hSession;
	HttpRequest::vector _requests;
	CRITICAL_SECTION _sync;
	DWORD _maxConnectionsPerServer;
public:
	Http();
	virtual ~Http();

	void SetMaxConnectionsPerServer(DWORD maxCon);
	BOOL Open(LPCWSTR pwszUserAgent, DWORD dwAccessType, LPCWSTR pwszProxyName, LPCWSTR pwszProxyBypass);
	BOOL DownloadFile(LPCWSTR url, HttpRequest *request);
	void Stop();
	void Close();

	void Lock();
	void Unlock();

private:
	static VOID CALLBACK HttpCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);

protected:
	void RemoveRequest(HttpRequest *request);
};

inline DWORD HttpRequest::GetStatusCode()
{
	return _status;
}

inline void HttpRequest::Lock()
{
	EnterCriticalSection(&_sync);
}

inline void HttpRequest::Unlock()
{
	LeaveCriticalSection(&_sync);
}

inline ULONG64 HttpRequest::GetContentLength()
{
	return _content_length;
}

inline void Http::Lock()
{
	EnterCriticalSection(&_sync);
}

inline void Http::Unlock()
{
	LeaveCriticalSection(&_sync);
}

inline void Http::SetMaxConnectionsPerServer(DWORD maxCon)
{
	_maxConnectionsPerServer = maxCon;
}
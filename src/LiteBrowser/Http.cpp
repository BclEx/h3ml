#include "Http.h"

#pragma comment(lib, "winhttp.lib")

Http::Http()
{
	_hSession = NULL;
	_maxConnectionsPerServer = 5;
	InitializeCriticalSectionAndSpinCount(&_sync, 1000);
}

Http::~Http()
{
	Stop();
	if (_hSession)
		WinHttpCloseHandle(_hSession);
	DeleteCriticalSection(&_sync);
}

BOOL Http::Open(LPCWSTR pwszUserAgent, DWORD dwAccessType, LPCWSTR pwszProxyName, LPCWSTR pwszProxyBypass)
{
	_hSession = WinHttpOpen(pwszUserAgent, dwAccessType, pwszProxyName, pwszProxyBypass, WINHTTP_FLAG_ASYNC);
	if (_hSession) {
		WinHttpSetOption(_hSession, WINHTTP_OPTION_MAX_CONNS_PER_SERVER, &_maxConnectionsPerServer, sizeof(_maxConnectionsPerServer));
		if (WinHttpSetStatusCallback(_hSession, (WINHTTP_STATUS_CALLBACK)Http::HttpCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0) != WINHTTP_INVALID_STATUS_CALLBACK)
			return TRUE;
	}
	if (_hSession)
		WinHttpCloseHandle(_hSession);
	return FALSE;
}

void Http::Close()
{
	if (_hSession) {
		WinHttpCloseHandle(_hSession);
		_hSession = NULL;
	}
}

VOID CALLBACK Http::HttpCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
	CoInitialize(NULL);
	DWORD dwError = ERROR_SUCCESS;
	HttpRequest *request = (HttpRequest *)dwContext;
	if (request) {
		switch (dwInternetStatus) {
		case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
			dwError = request->OnSendRequestComplete();
			break;
		case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
			dwError = request->OnHeadersAvailable();
			break;
		case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
			dwError = request->OnReadComplete(dwStatusInformationLength);
			break;
		case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
			dwError = request->OnHandleClosing();
			break;
		case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
			dwError = request->OnRequestError(((WINHTTP_ASYNC_RESULT*)lpvStatusInformation)->dwError);
			break;
		}
		if (dwError != ERROR_SUCCESS)
			request->Cancel();
	}
	CoUninitialize();
}

BOOL Http::DownloadFile(LPCWSTR url, HttpRequest *request)
{
	if (request) {
		request->SetParent(this);
		if (request->Create(url, _hSession)) {
			Lock();
			_requests.push_back(request);
			Unlock();
			return TRUE;
		}
	}
	return FALSE;
}

void Http::RemoveRequest(HttpRequest *request)
{
	bool isOk = false;
	Lock();
	for (HttpRequest::vector::iterator i = _requests.begin(); i != _requests.end(); i++) {
		if ((*i) == request) {
			_requests.erase(i);
			isOk = true;
			break;
		}
	}
	Unlock();
	if (isOk)
		request->Release();
}

void Http::Stop()
{
	Lock();
	for (HttpRequest::vector::iterator i = _requests.begin(); i != _requests.end(); i++)
		(*i)->Cancel();
	Unlock();
}

//////////////////////////////////////////////////////////////////////////

HttpRequest::HttpRequest()
{
	_status = 0;
	_error = 0;
	_downloaded_length = 0;
	_content_length = 0;
	_refCount = 1;
	_hConnection = NULL;
	_hRequest = NULL;
	_http = NULL;
	InitializeCriticalSection(&_sync);
}

HttpRequest::~HttpRequest()
{
	Cancel();
	DeleteCriticalSection(&_sync);
}

BOOL HttpRequest::Create(LPCWSTR url, HINTERNET hSession)
{
	_url = url;
	_error = ERROR_SUCCESS;

	URL_COMPONENTS urlComp;
	ZeroMemory(&urlComp, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.dwSchemeLength = -1;
	urlComp.dwHostNameLength = -1;
	urlComp.dwUrlPathLength = -1;
	urlComp.dwExtraInfoLength = -1;
	if (!WinHttpCrackUrl(url, lstrlen(url), 0, &urlComp))
		return FALSE;

	std::wstring host;
	std::wstring path;
	std::wstring extra;
	host.insert(0, urlComp.lpszHostName, urlComp.dwHostNameLength);
	path.insert(0, urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
	if (urlComp.dwExtraInfoLength)
		extra.insert(0, urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);

	DWORD flags = 0;
	if (urlComp.nScheme == INTERNET_SCHEME_HTTPS)
		flags = WINHTTP_FLAG_SECURE;

	_hConnection = WinHttpConnect(hSession, host.c_str(), urlComp.nPort, 0);

	PCWSTR pwszAcceptTypes[] = { L"*/*", NULL };
	path += extra;
	_hRequest = WinHttpOpenRequest(_hConnection, L"GET", path.c_str(), NULL, NULL, pwszAcceptTypes, flags);

	Lock();
	if (!_hRequest) {
		WinHttpCloseHandle(_hConnection);
		_hConnection = NULL;
		Unlock();
		return FALSE;
	}
	if (!WinHttpSendRequest(_hRequest, NULL, 0, NULL, 0, 0, (DWORD_PTR) this)) {
		WinHttpCloseHandle(_hRequest);
		_hRequest = NULL;
		WinHttpCloseHandle(_hConnection);
		_hConnection = NULL;
		Unlock();
		return FALSE;
	}
	Unlock();
	return TRUE;
}

void HttpRequest::Cancel()
{
	Lock();
	if (_hRequest) {
		WinHttpCloseHandle(_hRequest);
		_hRequest = NULL;
	}
	if (_hConnection) {
		WinHttpCloseHandle(_hConnection);
		_hConnection = NULL;
	}
	Unlock();
}

DWORD HttpRequest::OnSendRequestComplete()
{
	Lock();
	DWORD dwError = ERROR_SUCCESS;
	if (!WinHttpReceiveResponse(_hRequest, NULL))
		dwError = GetLastError();
	Unlock();
	return dwError;
}

DWORD HttpRequest::OnHeadersAvailable()
{
	Lock();
	DWORD dwError = ERROR_SUCCESS;
	_status = 0;
	DWORD StatusCodeLength = sizeof(_status);
	OnHeadersReady(_hRequest);
	if (!WinHttpQueryHeaders(_hRequest, WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE, NULL, &_status, &StatusCodeLength, NULL))
		dwError = GetLastError();
	else {
		WCHAR buf[255];
		DWORD len = sizeof(buf);
		if (WinHttpQueryHeaders(_hRequest, WINHTTP_QUERY_CONTENT_LENGTH, NULL, buf, &len, NULL))
			_content_length = _wtoi64(buf);
		else
			_content_length = 0;
		_downloaded_length = 0;
		dwError = ReadData();
	}
	Unlock();
	return dwError;
}

DWORD HttpRequest::OnHandleClosing()
{
	WCHAR errMsg[255];
	errMsg[0] = 0;
	if (_error)
		FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM, GetModuleHandle(L"winhttp.dll"), _error, 0, errMsg, 255, NULL);
	OnFinish(_error, errMsg);
	_http->RemoveRequest(this);
	return ERROR_SUCCESS;
}

DWORD HttpRequest::OnRequestError(DWORD dwError)
{
	_error = dwError;
	return _error;
}

DWORD HttpRequest::ReadData()
{
	DWORD dwError = ERROR_SUCCESS;
	if (!WinHttpReadData(_hRequest, _buffer, sizeof(_buffer), NULL))
		dwError = GetLastError();
	return dwError;
}

DWORD HttpRequest::OnReadComplete(DWORD len)
{
	DWORD dwError = ERROR_SUCCESS;
	if (len != 0) {
		Lock();
		_downloaded_length += len;
		OnData(_buffer, len, _downloaded_length, _content_length);
		dwError = ReadData();
		Unlock();
	}
	else
		Cancel();
	return dwError;
}

void HttpRequest::AddRef()
{
	InterlockedIncrement(&_refCount);
}

void HttpRequest::Release()
{
	LONG lRefCount;
	lRefCount = InterlockedDecrement(&_refCount);
	if (lRefCount == 0)
		delete this;
}

void HttpRequest::SetParent(Http *parent)
{
	_http = parent;
}

void HttpRequest::OnHeadersReady(HINTERNET hRequest)
{
}
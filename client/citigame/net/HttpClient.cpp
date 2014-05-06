#include "StdInc.h"
#include "HttpClient.h"
#include "fiDevice.h"
#include <sstream>

HttpClient::HttpClient()
{
	hWinHttp = WinHttpOpen(L"CitizenIV/1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
	WinHttpSetTimeouts(hWinHttp, 2000, 5000, 5000, 5000);
}

HttpClient::~HttpClient()
{
	WinHttpCloseHandle(hWinHttp);
}

void HttpClient::DoPostRequest(std::wstring host, uint16_t port, std::wstring url, std::map<std::string, std::string>& fields, std::function<void(bool, std::string)> callback)
{
	std::string postData = BuildPostString(fields);

	DoPostRequest(host, port, url, postData, callback);
}

struct HttpClientRequestContext
{
	HttpClient* client;
	HINTERNET hConnection;
	HINTERNET hRequest;
	std::string postData;
	std::function<void(bool, std::string)> callback;

	std::string resultData;
	char buffer[32768];

	rage::fiDevice* outDevice;
	int outHandle;

	HttpClientRequestContext()
		: outDevice(nullptr)
	{

	}

	void DoCallback(bool success, std::string& resData)
	{
		callback(success, resData);

		if (outDevice)
		{
			outDevice->close(outHandle);
		}

		WinHttpCloseHandle(hConnection);
		WinHttpCloseHandle(hRequest);

		delete this;
	}
};

void HttpClient::DoPostRequest(std::wstring host, uint16_t port, std::wstring url, std::string postData, std::function<void(bool, std::string)> callback)
{
	HINTERNET hConnection = WinHttpConnect(hWinHttp, host.c_str(), port, 0);
	HINTERNET hRequest = WinHttpOpenRequest(hConnection, L"POST", url.c_str(), 0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

	WinHttpSetStatusCallback(hRequest, StatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);

	HttpClientRequestContext* context = new HttpClientRequestContext;
	context->client = this;
	context->hConnection = hConnection;
	context->hRequest = hRequest;
	context->postData = postData;
	context->callback = callback;

	WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, const_cast<char*>(context->postData.c_str()), context->postData.length(), context->postData.length(), (DWORD_PTR)context);
}

void HttpClient::DoFileGetRequest(std::wstring host, uint16_t port, std::wstring url, std::string outFilename, std::function<void(bool, std::string)> callback)
{
	HINTERNET hConnection = WinHttpConnect(hWinHttp, host.c_str(), port, 0);
	HINTERNET hRequest = WinHttpOpenRequest(hConnection, L"GET", url.c_str(), 0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

	WinHttpSetStatusCallback(hRequest, StatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);

	HttpClientRequestContext* context = new HttpClientRequestContext;
	context->client = this;
	context->hConnection = hConnection;
	context->hRequest = hRequest;
	context->callback = callback;
	context->outDevice = rage::fiDevice::GetDevice(outFilename.c_str(), true);
	context->outHandle = context->outDevice->create(outFilename.c_str());

	WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, (DWORD_PTR)context);
}

void HttpClient::StatusCallback(HINTERNET handle, DWORD_PTR context, DWORD code, void* info, DWORD length)
{
	HttpClientRequestContext* ctx = (HttpClientRequestContext*)context;

	switch (code)
	{
		case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
			ctx->DoCallback(false, std::string());
			break;

		case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
			if (!WinHttpReceiveResponse(ctx->hRequest, 0))
			{
				ctx->DoCallback(false, std::string());
			}

			break;

		case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
		{
			uint32_t statusCode;
			DWORD statusCodeLength = sizeof(uint32_t);

			if (!WinHttpQueryHeaders(ctx->hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeLength, WINHTTP_NO_HEADER_INDEX))
			{
				ctx->DoCallback(false, std::string());
				return;
			}

			if (statusCode != HTTP_STATUS_OK)
			{
				ctx->DoCallback(false, std::string());
				return;
			}

			if (!WinHttpReadData(ctx->hRequest, ctx->buffer, sizeof(ctx->buffer) - 1, nullptr))
			{
				ctx->DoCallback(false, std::string());
			}

			break;
		}
		case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
			if (ctx->outDevice)
			{
				ctx->outDevice->write(ctx->outHandle, ctx->buffer, length);
			}
			else
			{
				ctx->buffer[length] = '\0';

				ctx->resultData += ctx->buffer;
			}

			if (length > 0)
			{
				if (!WinHttpReadData(ctx->hRequest, ctx->buffer, sizeof(ctx->buffer) - 1, nullptr))
				{
					ctx->DoCallback(false, std::string());
				}
			}
			else
			{
				ctx->DoCallback(true, ctx->resultData);
			}
			
			break;
	}
}

bool HttpClient::CrackUrl(std::string url, std::wstring& hostname, std::wstring& path, uint16_t& port)
{
	wchar_t wideUrl[1024];
	mbstowcs(wideUrl, url.c_str(), _countof(wideUrl));
	wideUrl[1023] = L'\0';

	URL_COMPONENTS components = { 0 };
	components.dwStructSize = sizeof(components);

	components.dwHostNameLength = -1;
	components.dwUrlPathLength = -1;
	components.dwExtraInfoLength = -1;

	if (!WinHttpCrackUrl(wideUrl, wcslen(wideUrl), 0, &components))
	{
		return false;
	}

	hostname = std::wstring(components.lpszHostName, components.dwHostNameLength);
	path = std::wstring(components.lpszUrlPath, components.dwUrlPathLength);
	path += std::wstring(components.lpszExtraInfo, components.dwExtraInfoLength);
	port = components.nPort;

	return true;
}

// TODO: urlencode?
std::string HttpClient::BuildPostString(std::map<std::string, std::string>& fields)
{
	std::stringstream retval;
	
	for (auto& field : fields)
	{
		retval << field.first << "=" << field.second << "&";
	}

	std::string str = retval.str();
	return str.substr(0, str.length() - 1);
}
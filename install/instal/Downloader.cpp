#include "Downloader.h"
#include <Wininet.h>
#include <fstream>

Downloader::Downloader()
{
	callback.done_ = false;
	callback.is_ok_ = false;
	callback.prorgess_ = 0;
	callback.total_ = 100;
}

Downloader::~Downloader()
{
}

bool Downloader::is_done()
{
	return callback.done_;
}

bool Downloader::is_ok()
{
	return callback.is_ok_;
}

void Downloader::downlaod(const wchar_t *url, const wchar_t *path)
{
	DeleteUrlCacheEntry(url);
	DWORD state = URLDownloadToFile(NULL, url, path, 0, &callback);
	callback.is_ok_ = state == S_OK;
	callback.done_ = true;
}

void Downloader::get_prorgess(unsigned int *total, unsigned int *done)
{
	*total = callback.total_;
	*done = callback.prorgess_;
}

Downloader::CCallback::CCallback()
{
}

Downloader::CCallback::~CCallback()
{
}

HRESULT Downloader::CCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
	ULONG ulStatusCode, LPCWSTR wszStatusText)
{
	total_ = ulProgressMax;
	prorgess_ = ulProgress;
	return S_OK;
}
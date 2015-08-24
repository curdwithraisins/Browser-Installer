#pragma once

#include <windows.h>

class Downloader
{
public:
	Downloader();
	~Downloader();

	void downlaod(const wchar_t *url, const wchar_t *path);
	void get_prorgess(unsigned int *total, unsigned int *done);
	bool is_done();
	bool is_ok();

private:

	class CCallback : public IBindStatusCallback
	{
	public:
		CCallback();
		~CCallback();

		volatile int prorgess_;
		volatile int total_;
		volatile bool done_;
		volatile bool is_ok_;

		STDMETHOD(OnStartBinding)(
			/* [in] */ DWORD dwReserved,
			/* [in] */ IBinding __RPC_FAR *pib)
		{
			return E_NOTIMPL;
		}

		STDMETHOD(GetPriority)(
			/* [out] */ LONG __RPC_FAR *pnPriority)
		{
			return E_NOTIMPL;
		}

		STDMETHOD(OnLowResource)(
			/* [in] */ DWORD reserved)
		{
			return E_NOTIMPL;
		}

		STDMETHOD(OnProgress)(
			/* [in] */ ULONG ulProgress,
			/* [in] */ ULONG ulProgressMax,
			/* [in] */ ULONG ulStatusCode,
			/* [in] */ LPCWSTR wszStatusText);

		STDMETHOD(OnStopBinding)(
			/* [in] */ HRESULT hresult,
			/* [unique][in] */ LPCWSTR szError)
		{
			return E_NOTIMPL;
		}

		STDMETHOD(GetBindInfo)(
			/* [out] */ DWORD __RPC_FAR *grfBINDF,
			/* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
		{
			return E_NOTIMPL;
		}

		STDMETHOD(OnDataAvailable)(
			/* [in] */ DWORD grfBSCF,
			/* [in] */ DWORD dwSize,
			/* [in] */ FORMATETC __RPC_FAR *pformatetc,
			/* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
		{
			return E_NOTIMPL;
		}

		STDMETHOD(OnObjectAvailable)(
			/* [in] */ REFIID riid,
			/* [iid_is][in] */ IUnknown __RPC_FAR *punk)
		{
			return E_NOTIMPL;
		}

		STDMETHOD_(ULONG, AddRef)()
		{
			return 0;
		}

		STDMETHOD_(ULONG, Release)()
		{
			return 0;
		}

		STDMETHOD(QueryInterface)(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
		{
			return E_NOTIMPL;
		}
	};

	CCallback callback;
};


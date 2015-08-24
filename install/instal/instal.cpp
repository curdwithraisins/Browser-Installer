#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include <iostream>
#include <tchar.h>
#include "unzip.h"
#include "Downloader.h"
#include <objidl.h> 
#include <shlobj.h> 
#include <algorithm>
#include <vector>

HINSTANCE hIns;
DWORD ZipCount = 0;
DWORD ZipTotal = 0;
DWORD RegProc = 0;
Downloader downlader;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//второй поток
DWORD WINAPI ThreadProc(LPVOID);
//создание ярлыка
void CreateLink(TCHAR*);
//программы и компоненты
void ProgramsandFeatures(TCHAR*);
//дефолтный браузер
void RunnerExec(const std::wstring &, bool);

// копирование истории и закладок
void CopyHistory();

std::wstring FindBiggestVersionName();

int GetScaledProgress(int min, int max)
{
	int scale_len = (max - min) / 2;
	if (downlader.is_done())
	{
		if (ZipTotal == 0) return min + scale_len;
		return min + scale_len + ZipCount * scale_len / ZipTotal;
	}
	else
	{
		unsigned int curret, total;
		downlader.get_prorgess(&total, &curret);
		if (total == 0) return min;
		return min + curret * scale_len / total;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	char *szAppName = "installer";
	HWND hwnd;
	MSG msg;
	WNDCLASSEX wndclass;

	hIns = hInstance;

	wndclass.cbSize = sizeof (wndclass);
	wndclass.style = CS_BYTEALIGNWINDOW | CS_OWNDC | CS_DROPSHADOW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = LPCWSTR(szAppName);
	wndclass.hIconSm = NULL;

	if (!RegisterClassEx(&wndclass))
	{
		MessageBox(NULL, LPCWSTR(L"Cennot register class"), LPCWSTR(L"Error"), MB_OK);
		return 0;
	}

	//размер экрана для центрирования окна
	HDC ScreenDC;
	int screenx, screeny;
	ScreenDC = GetDC(0);
	screenx = GetDeviceCaps(ScreenDC, HORZRES);
	screeny = GetDeviceCaps(ScreenDC, VERTRES);
	screenx = screenx / 2 - 570 / 2;
	screeny = screeny / 2 - 225 / 2;

	hwnd = CreateWindow(LPCWSTR(szAppName), LPCWSTR(""), WS_VISIBLE,
		screenx, screeny, 570, 225, NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		wchar_t buff[100];
		_itow_s(GetLastError(), buff, 10);
		MessageBox(NULL, LPCWSTR(L"Can not create window"), buff, MB_OK);
		return 0;
	}
	SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & (~WS_CAPTION));
	UpdateWindow(hwnd);

	//новый поток
	DWORD dwThreadID;
	HANDLE hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &dwThreadID);

	if (hThread == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, LPCWSTR("Cennot create thread"), LPCWSTR("Error"), MB_OK);
		return 0;
	}
	
	//таймер для отслеживания изменений процесса на 100мс
	SetTimer(hwnd, 1, 100, NULL);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT PaintStruct;
	RECT Rect;
	static HBITMAP hBitmap;
	BITMAP Bitmap;
	static HDC hCompatibleDC;
	static int x, y, xn, yn, process, l;
	static bool FlagMove = false, FlagRed = false, FlagBlue = false;
	static LANGID lang;
	static HBRUSH hBrushGrey, hBrushRed, hBrushWhite; //для индикации процесса
	static LOGFONT font;
	static HFONT hfonttext, hfontdash;
	DWORD Count;
	HRSRC Resource;
	DWORD Length;
	HGLOBAL ResourceData;
	static HANDLE hFont;
	static DWORD ZipCountPlus = 0;
	PVOID FontData;
	static const wchar_t *InstallText[][2] = {
		{ L"Загрузка", L"Установка файлов" },
		{ L"Downloading", L"Installing Wind browser" }
	};

	HDC          hdcMem;
	HBITMAP      hbmMem;
	HANDLE       hOld;
	int win_width, win_height;


	switch (iMsg)
	{
	case WM_CREATE:

		//шрифт
		Count = 0;
		Resource = FindResource(NULL, MAKEINTRESOURCE(IDR_FONT1), RT_FONT);
		Length = SizeofResource(NULL, Resource);
		ResourceData = LoadResource(NULL, Resource);
		FontData = LockResource(ResourceData);
		hFont = AddFontMemResourceEx(FontData, Length, 0, &Count);
		if (hFont != 0)
		{
			SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
			lstrcpy((LPWSTR)&font.lfFaceName, L"InformaPro-Light");
		}
		else
			MessageBox(hwnd, L"Font 1 Error", L"error", MB_OK);
		font.lfHeight = 38; 
		hfonttext = CreateFontIndirect(&font);
		font.lfHeight = 30;
		hfontdash = CreateFontIndirect(&font);


		//загрузка лого
		hBitmap = LoadBitmap(hIns, MAKEINTRESOURCE(IDB_BITMAP1));
		hDC = GetDC(hwnd);
		hCompatibleDC = CreateCompatibleDC(hDC);
		ReleaseDC(hwnd, hDC);

		//кисть для прямоугольников загрузки
		hBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
		hBrushGrey = CreateSolidBrush(RGB(218, 214, 214));
		hBrushRed = CreateSolidBrush(RGB(255, 40, 86));

		break;

	case WM_PAINT:
		hDC = BeginPaint(hwnd, &PaintStruct);

		GetWindowRect(hwnd, &Rect);
		win_width = Rect.right - Rect.left;
		win_height = Rect.bottom - Rect.top;

		// Create an off-screen DC for double-buffering
		hdcMem = CreateCompatibleDC(hDC);
		hbmMem = CreateCompatibleBitmap(hDC, win_width, win_height);
		hOld = SelectObject(hdcMem, hbmMem);
		std::swap(hDC, hdcMem);
		// double-buffered painting.................

		Rect.top = 0;
		Rect.left = 0;
		Rect.right = win_width;
		Rect.bottom = win_height;

		FillRect(hDC, &Rect, hBrushWhite);

		//вывод лого
		SelectObject(hCompatibleDC, hBitmap);
		GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		StretchBlt(hDC, 233, 34, Bitmap.bmWidth, Bitmap.bmHeight, hCompatibleDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SRCCOPY);
		
		//текст
		SelectObject(hDC, hfonttext);
		Rect.top = 130;
		Rect.left = 0;
		Rect.bottom = 185;
		Rect.right = 570;		

		process = GetScaledProgress(0, 100);

		//локализация
		lang = GetSystemDefaultUILanguage(); //0x0419/1049 - русский

		if (lang == 0x0419)
			DrawText(hDC, InstallText[0][process / 51], wcslen(InstallText[0][process / 51]), &Rect, DT_TOP || DT_CENTER);
		else
			DrawText(hDC, InstallText[1][process / 51], wcslen(InstallText[1][process / 51]), &Rect, DT_TOP || DT_CENTER);
	
		//вывод процесса
		Rect.top = 193;
		Rect.left = 9;
		Rect.bottom = 197;
		Rect.right = 559;
		FillRect(hDC, &Rect, hBrushGrey);
		//550 - длина полосы в пикселях. 500 - скачка и распаковка
		//1% = 5п
		Rect.right = (process + RegProc) * 5 + 10;
		FillRect(hDC, &Rect, hBrushRed);
		
		//_
		SelectObject(hDC, hfontdash);
		if (FlagBlue)
			SetTextColor(hDC, RGB(10, 100, 250));
		TextOut(hDC, 530, 0, L"_", 1);
		
		// end double-buffered painting.................
		std::swap(hDC, hdcMem);
		BitBlt(hDC, 0, 0, win_width, win_height, hdcMem, 0, 0, SRCCOPY);
		// Free-up the off-screen DC
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);

		EndPaint(hwnd, &PaintStruct);
		
		break;

	case WM_MOUSEMOVE:
		//если навели на _
		if ((HIWORD(lParam) < 40) && (LOWORD(lParam) > 520) && (!FlagBlue))
		{
			FlagBlue = true;
			InvalidateRect(hwnd, NULL, true);
		}
		if (FlagBlue)
			if ((HIWORD(lParam) > 40) || (LOWORD(lParam) < 520))
			{
				FlagBlue = false;
				InvalidateRect(hwnd, NULL, true);
			}

		//перетаскивание
		xn = LOWORD(lParam); //положение курсора по х
		yn = HIWORD(lParam); //положение курсора по у		
		if (FlagMove)
		{
			GetWindowRect(hwnd, &Rect);
			int xnx = Rect.left - x + xn;
			int yny = Rect.top - y + yn;
			MoveWindow(hwnd, xnx, yny, 570, 225, true);
			InvalidateRect(hwnd, NULL, true);
		}

		break;

	case WM_LBUTTONDOWN:
		x = LOWORD(lParam); //положение курсора по х
		y = HIWORD(lParam); //положение курсора по у
		if ((x > 520) && (y < 40))
			ShowWindow(hwnd, SW_MINIMIZE);
		else
			FlagMove = true;

		break;

	case WM_LBUTTONUP:
		if (FlagMove)
			FlagMove = false;

		break;

	case WM_TIMER:
		InvalidateRect(hwnd, NULL, true);
		break;

	case WM_DESTROY:
		RemoveFontMemResourceEx(hFont);
		PostQuitMessage(0);

		break;
	default:return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID t)
{
	TCHAR temppath[MAX_PATH] = {0};
	GetTempPath(MAX_PATH, temppath);
	std::wstring zip_name(temppath);
	zip_name  += L"instll_wind.zip";
	
	DeleteFile(zip_name.c_str());
	std::wstring url = L"http://files.light-keeper.net/shared/.chromium/release/install.zip";
	downlader.downlaod(url.c_str(), zip_name.c_str());
	
	if (downlader.is_ok() == false)
	{
		MessageBox(NULL, L"Не удалось скачать инсталятор", L"Ошибка", NULL);
		return 0;
	}
	
	HZIP hz = OpenZip(zip_name.c_str(), 0);
	ZIPENTRY ze;

	GetZipItem(hz, -1, &ze); 	
	int numitems = ze.index;
	ZipTotal = numitems;	
	
	TCHAR stol[MAX_PATH];
	LPITEMIDLIST pidl;

	//AppData\\Local
	SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);
	SHGetPathFromIDList(pidl, stol);
	
	TCHAR dest[MAX_PATH];
	swprintf(stol, MAX_PATH, L"%s\\%s", stol, L"Wind");
	
	for (int zi = 0; zi < numitems; zi++)
	{
		ZipCount++;
		GetZipItem(hz, zi, &ze);
		swprintf(dest, MAX_PATH, L"%s\\%s", stol, ze.name);
		UnzipItem(hz, zi, dest);
	}

	CloseZip(hz);
	DeleteFile(zip_name.c_str());

	//ярлыки и пуск
	CreateLink(stol);
	//копировать историю
	CopyHistory();
	//редактор реестра
	ProgramsandFeatures(stol);


	HKEY hKey;
	TCHAR pvData[MAX_PATH];
	DWORD Buf = 8192;

	RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Clients\\StartMenuInternet", 0, KEY_ALL_ACCESS, &hKey);
	RegGetValue(hKey, L"", 0, REG_SZ, NULL, pvData, &Buf); //в pvData - дефолтный браузер
	RegCloseKey(hKey);

	wchar_t reg[MAX_PATH];
	wchar_t ccmd[MAX_PATH];
	SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);
	SHGetPathFromIDList(pidl, reg);
	swprintf(ccmd, MAX_PATH, L"\"%s\\Wind\\%s\\wind.exe\" --make-default-browser", reg, FindBiggestVersionName().c_str());
	RunnerExec(ccmd, true);

	swprintf(ccmd, MAX_PATH, L"\"%s\\%s", reg, L"Wind\\launcher.exe\" ");
	RunnerExec(ccmd, false);

	ExitProcess(0);

	return 0;
}

void CreateLink(TCHAR* stol)
{
	LPITEMIDLIST pidl;
	TCHAR desk[MAX_PATH], dest[MAX_PATH];

	//создание ярлыка
	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &pidl);
	SHGetPathFromIDList(pidl, desk);

	HRESULT hres;
	IShellLink *NewLink;

	CoInitialize(NULL);

	hres = CoCreateInstance(CLSID_ShellLink, NULL,CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&NewLink);
	if (SUCCEEDED(hres))
	{
		//из Local\\Wind
		swprintf(dest, MAX_PATH, L"%s\\%s", stol, L"launcher.exe");

		NewLink->SetPath(dest); //что
		NewLink->SetWorkingDirectory(desk); //куда
		NewLink->SetDescription(L"Wind browser"); //описание
		NewLink->SetShowCmd(SW_SHOW);
		IPersistFile *ppf;

		NewLink->QueryInterface(IID_IPersistFile, (void**)&ppf);

		swprintf(dest, MAX_PATH, L"%s\\%s", desk, L"Wind.lnk"); //link
		ppf->Save(dest, true);
		ppf->Release();

		SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidl);
		SHGetPathFromIDList(pidl, desk);
		std::wstring new_path = desk;
		new_path += L"\\Wind browser";
		CreateDirectory(new_path.c_str(), 0);

		NewLink->SetWorkingDirectory(desk); //куда

		swprintf(dest, MAX_PATH, L"%s\\%s", new_path.c_str(), L"Wind.lnk"); //link
		ppf->Save(dest, true);

		swprintf(dest, MAX_PATH, L"%s\\%s", stol, L"uninstall.exe");
		NewLink->SetPath(dest); //что
		NewLink->QueryInterface(IID_IPersistFile, (void**)&ppf);

		swprintf(dest, MAX_PATH, L"%s\\%s", new_path.c_str(), L"uninstall.lnk"); //link
		ppf->Save(dest, true);


		ppf->Release();
	}

	CoUninitialize();
}

void ProgramsandFeatures(TCHAR* stol)
{
	HKEY hKey;
	DWORD dwDisposition;
	TCHAR text[MAX_PATH];
	LONG rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Wind",0,
		NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	if (rc == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, L"DisplayName", 0, REG_SZ, (LPBYTE)L"Wind", 16);
		RegSetValueEx(hKey, L"Publisher", 0, REG_SZ, (LPBYTE)L"WindCompany", 16);
		swprintf(text, MAX_PATH, L"%s\\%s", stol, L"launcher.exe");
		RegSetValueEx(hKey, L"DisplayIcon", 0, REG_SZ, (LPBYTE)text, lstrlen(text) * 2);
		swprintf(text, MAX_PATH, L"%s\\%s", stol, L"uninstall.exe");
		RegSetValueEx(hKey, L"UninstallString", 0, REG_SZ, (LPBYTE)text, lstrlen(text) * 2);
		
		RegSetValueEx(hKey, L"DisplayVersion", 0, REG_SZ, (LPBYTE)FindBiggestVersionName().c_str(), lstrlen(FindBiggestVersionName().c_str()) * 2);
		RegSetValueEx(hKey, L"InstallLocation", 0, REG_SZ, (LPBYTE)stol, lstrlen(stol) * 2);
		RegCloseKey(hKey);
	}
	else
	{
		MessageBox(NULL, L"Programs and Features not changed", L"Error", MB_OK);
	}

}

void CopyHistory()
{
	HKEY hKey;
	TCHAR pvData[MAX_PATH];
	DWORD Buf = 8192;

	RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Clients\\StartMenuInternet", 0, KEY_ALL_ACCESS, &hKey);
	RegGetValue(hKey, L"", 0, REG_SZ, NULL, pvData, &Buf); //в pvData - дефолтный браузер
	RegCloseKey(hKey);

	std::wstring name = pvData;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	static const wchar_t *map[][2] = {
		{ L"yandex", L"Yandex\\YandexBrowser" },
		{ L"chromium", L"Chromium" },
		{ L"orbitum", L"Orbitum" },
		{ L"vivaldi", L"Vivaldi" },
		{ L"интернет", L"Xpom" }
	};

	TCHAR stol[MAX_PATH];
	LPITEMIDLIST pidl;

	SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);
	SHGetPathFromIDList(pidl, stol);

	std::wstring path_from;

	for (int i = 0; i < sizeof(map) / sizeof(map[0]); i++)
	{
		std::wstring in(map[i][0]);
		std::wstring p(map[i][1]);

		if (name.find(in) != std::wstring::npos)
		{
			path_from = stol;
			path_from += L"\\";
			path_from += p;
			path_from += L"\\User Data\\Default";
			break;
		}
	}
	
	if (path_from.empty()) return;

	std::wstring path_to(stol);
	path_to += L"\\Wind";
	CreateDirectory(path_to.c_str(), 0);
	path_to += L"\\User Data";
	CreateDirectory(path_to.c_str(), 0);
	path_to += L"\\Default";
	CreateDirectory(path_to.c_str(), 0);

	CopyFile((path_from + L"\\Bookmarks").c_str(), (path_to + L"\\Bookmarks").c_str(), false);
	CopyFile((path_from + L"\\History").c_str(), (path_to + L"\\History").c_str(), false);

}

void RunnerExec(const std::wstring &wcommand, bool wait)
{
	PROCESS_INFORMATION ProcessInfo; //This is what we get as an [out] parameter

	STARTUPINFOW StartupInfo; //This is an [in] parameter

	ZeroMemory(&StartupInfo, sizeof(StartupInfo));
	StartupInfo.cb = sizeof StartupInfo; //Only compulsory field

	if (CreateProcessW(NULL, (wchar_t *)wcommand.c_str(),
		NULL, NULL, FALSE, 0, NULL,
		NULL, &StartupInfo, &ProcessInfo))
	{
		if (wait)
		{
			WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
		}
		CloseHandle(ProcessInfo.hThread);
		CloseHandle(ProcessInfo.hProcess);
	}
}


std::wstring FindBiggestVersionName()
{
	std::wstring current_version_folder = L"";
	int current_version[4] = { 0 };
	WIN32_FIND_DATA data;
	
	LPITEMIDLIST pidl;
	TCHAR stol[MAX_PATH];

	SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);
	SHGetPathFromIDList(pidl, stol);

	std::wstring path(stol);
	path += L"\\Wind\\*.*.*.*";

	HANDLE h = FindFirstFile(path.c_str(), &data);

	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				int new_version[4] = { 0 };
				if (4 == swscanf_s(data.cFileName, L"%d.%d.%d.%d", &new_version[0], &new_version[1], &new_version[2], &new_version[3]))
				{
					int cmp = 0;
					for (int i = 0; i < 4 && cmp == 0; i++)
					{
						cmp = new_version[i] - current_version[i];
					}

					if (cmp > 0)
					{
						memcpy_s(current_version, sizeof(current_version), new_version, sizeof(new_version));
						current_version_folder = data.cFileName;
					}
				}
			}
		} while (FindNextFile(h, &data));

		FindClose(h);
	}

	return current_version_folder;
}

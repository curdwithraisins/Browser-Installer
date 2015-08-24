#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <objidl.h> 
#include <shlobj.h> 
#include <vector>

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

void Del(LPCTSTR szInDirName)
{
	HANDLE hFind;
	WIN32_FIND_DATA ffd;
	TCHAR destIn[MAX_PATH], destOut[MAX_PATH], destFind[MAX_PATH];
	sprintf_s(destFind, "%s\\%s", szInDirName, L"*.*");
	hFind = FindFirstFile(destFind, &ffd);
	do {
		sprintf_s(destIn, "%s\\%s", szInDirName, ffd.cFileName); //источник
		if (ffd.dwFileAttributes & 0x00000010)
		{
			if ((lstrcmp(ffd.cFileName, ".") == 0) || (lstrcmp(ffd.cFileName, "..") == 0))
				continue;
			Del(destIn);
			RemoveDirectory(destIn);
		}
		if (ffd.cFileName != "uninstal.exe")
			DeleteFile(destIn);
	} while (FindNextFile(hFind, &ffd));
	FindClose(hFind);
}

void main(int argc, char* argv[])
{
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);

	if (argc == 1)
	{

		LPITEMIDLIST pidl;
		TCHAR desk[MAX_PATH];
		std::wstring a;

		//определяем расположение папки Desktop
		SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &pidl);
		SHGetPathFromIDList(pidl, desk);

		lstrcat(desk, "\\Wind.lnk");
		DeleteFile(desk);

		//пуск
		SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidl);
		SHGetPathFromIDList(pidl, desk);
		lstrcat(desk, "\\Wind browser");
		int i = lstrlen(desk);
		desk[i + 1] = 0;
		desk[i + 2] = 0;

		SHFILEOPSTRUCT file_op = {
			NULL,
			FO_DELETE,
			desk,
			"",
			FOF_NOCONFIRMATION |
			FOF_NOERRORUI |
			FOF_SILENT,
			false,
			0,
			"" };
		SHFileOperation(&file_op);

		//находим папку в Local
		SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);
		SHGetPathFromIDList(pidl, desk);
		lstrcat(desk, "\\Wind");

		Del(desk);

		//удаление из реестра
		RegDeleteKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Wind",
			KEY_WOW64_32KEY, 0);

		HKEY hKey;
		wchar_t pvData[MAX_PATH];
		DWORD Buf = 8192;

		RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Clients\\StartMenuInternet\\IEXPLORE.EXE\\shell\\open\\command", 0, KEY_ALL_ACCESS, &hKey);
		RegGetValue(hKey, "", 0, REG_SZ, NULL, pvData, &Buf); //адрес ехе IEXPLORE
		RegCloseKey(hKey);

		/*wchar_t ccmd[MAX_PATH] = L" http://www.yandex.ru";
		wcscat_s(pvData, ccmd);
		RunnerExec(pvData, false);
		*/
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		// Make a copy of ourselves which we'll use to delete the version we were run from
		CopyFile(argv[0], "1.exe", FALSE);

		// Rename the running copy of ourself to another name
		MoveFile(argv[0], "2.exe");

		// Make sure we delete the copy of ourselves that's going to delete us when we die
		CreateFile("1.exe", 0, FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);

		// Invoke the process that will delete us
		// allowing it to inherit the handle we just created above.
		CreateProcess(NULL, "1.exe x", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	}
	else if (argc == 2)
	{
		// Wait for the original program to die (deleting us and closing a handle), then delete it
		while (!DeleteFile("2.exe"));

		// Launch a child process which will inherit our file handles
		// -- This keeps the file handle with FILE_FLAG_DELETE_ON_CLOSE (which we inherited) alive beyond our lifetime
		// this allowing us to be deleted after we've died and our own handle is closed.
		CreateProcess(NULL, "notepad", NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi);
	}
}
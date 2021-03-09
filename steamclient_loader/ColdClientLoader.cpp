// My own modified version of ColdClientLoader originally written by Rat431

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>

bool IsNotRelativePathOrRemoveFileName(CHAR* output, bool Remove)
{
	int LG = lstrlenA(output);
	for (int i = LG; i > 0; i--) {
		if (output[i] == '\\') {
			if(Remove)
				RtlFillMemory(&output[i], LG - i, NULL);
			return true;
		}
	}
	return false;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	CHAR CurrentDirectory[MAX_PATH] = { 0 };
	CHAR Client64Path[MAX_PATH] = { 0 };
	CHAR ClientPath[MAX_PATH] = { 0 };
	CHAR ExeFile[MAX_PATH] = { 0 };
	CHAR ExeRunDir[MAX_PATH] = { 0 };
	CHAR ExeCommandLine[4096] = { 0 };
	CHAR AppId[128] = { 0 };

	STARTUPINFOA info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	int Length = GetModuleFileNameA(GetModuleHandleA(NULL), CurrentDirectory, sizeof(CurrentDirectory)) + 1;
	for (int i = Length; i > 0; i--) {
		if (CurrentDirectory[i] == '\\') {
			lstrcpyA(&CurrentDirectory[i + 1], "ColdClientLoader.ini");
			break;
		}
	}
	if (GetFileAttributesA(CurrentDirectory) == INVALID_FILE_ATTRIBUTES) {
		MessageBoxA(NULL, "Couldn't find the configuration file(ColdClientLoader.ini).", "ColdClientLoader", MB_ICONERROR);
		return 0;
	}

	GetPrivateProfileStringA("SteamClient", "SteamClient64Dll", "", Client64Path, MAX_PATH, CurrentDirectory);
	GetPrivateProfileStringA("SteamClient", "SteamClientDll", "", ClientPath, MAX_PATH, CurrentDirectory);
	GetPrivateProfileStringA("SteamClient", "Exe", NULL, ExeFile, MAX_PATH, CurrentDirectory);
	GetPrivateProfileStringA("SteamClient", "ExeRunDir", NULL, ExeRunDir, MAX_PATH, CurrentDirectory);
	GetPrivateProfileStringA("SteamClient", "ExeCommandLine", NULL, ExeCommandLine, 4096, CurrentDirectory);
	GetPrivateProfileStringA("SteamClient", "AppId", NULL, AppId, sizeof(AppId), CurrentDirectory);

	if (AppId[0]) {
		SetEnvironmentVariableA("SteamAppId", AppId);
		SetEnvironmentVariableA("SteamGameId", AppId);
	}

	CHAR TMP[MAX_PATH] = {};
	if (!IsNotRelativePathOrRemoveFileName(Client64Path, false)) {
		lstrcpyA(TMP, Client64Path);
		ZeroMemory(Client64Path, sizeof(Client64Path));
		GetFullPathNameA(TMP, MAX_PATH, Client64Path, NULL);
	}
	if (!IsNotRelativePathOrRemoveFileName(ClientPath, false)) {
		lstrcpyA(TMP, ClientPath);
		ZeroMemory(ClientPath, sizeof(ClientPath));
		GetFullPathNameA(TMP, MAX_PATH, ClientPath, NULL);
	}
	if (!IsNotRelativePathOrRemoveFileName(ExeFile, false)) {
		lstrcpyA(TMP, ExeFile);
		ZeroMemory(ExeFile, sizeof(ExeFile));
		GetFullPathNameA(TMP, MAX_PATH, ExeFile, NULL);
	}
	if (!IsNotRelativePathOrRemoveFileName(ExeRunDir, false)) {
		lstrcpyA(TMP, ExeRunDir);
		ZeroMemory(ExeRunDir, sizeof(ExeRunDir));
		GetFullPathNameA(TMP, MAX_PATH, ExeRunDir, NULL);
	}

	if (GetFileAttributesA(Client64Path) == INVALID_FILE_ATTRIBUTES) {
		MessageBoxA(NULL, "Couldn't find the requested SteamClient64Dll.", "ColdClientLoader", MB_ICONERROR);
		return 0;
	}

	if (GetFileAttributesA(ClientPath) == INVALID_FILE_ATTRIBUTES) {
		MessageBoxA(NULL, "Couldn't find the requested SteamClientDll.", "ColdClientLoader", MB_ICONERROR);
		return 0;
	}

	if (GetFileAttributesA(ExeFile) == INVALID_FILE_ATTRIBUTES) {
		MessageBoxA(NULL, "Couldn't find the requested Exe file.", "ColdClientLoader", MB_ICONERROR);
		return 0;
	}

	CHAR CommandLine[8192];
	snprintf(CommandLine, sizeof(CommandLine), "\"%s\" %s", ExeFile, ExeCommandLine);
	if (!ExeFile[0] || !CreateProcessA(ExeFile, CommandLine, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, ExeRunDir, &info, &processInfo))
	{
		MessageBoxA(NULL, "Unable to load the requested EXE file.", "ColdClientLoader", MB_ICONERROR);
		return 0;
	}
	HKEY Registrykey;
	// Declare some variables to be used for Steam registry.
	DWORD UserId = 0x03100004771F810D & 0xffffffff;
	DWORD ProcessID = GetCurrentProcessId();

	bool orig_steam = false;
	DWORD keyType = REG_SZ;
	CHAR OrgSteamCDir[MAX_PATH] = { 0 };
	CHAR OrgSteamCDir64[MAX_PATH] = { 0 };
	DWORD Size1 = MAX_PATH;
	DWORD Size2 = MAX_PATH;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_ALL_ACCESS, &Registrykey) == ERROR_SUCCESS)
	{
		orig_steam = true;
		// Get original values to restore later.
		RegQueryValueExA(Registrykey, "SteamClientDll", 0, &keyType, (LPBYTE)& OrgSteamCDir, &Size1);
		RegQueryValueExA(Registrykey, "SteamClientDll64", 0, &keyType, (LPBYTE)& OrgSteamCDir64, &Size2);
	} else {
		if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, 0, REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, NULL, &Registrykey, NULL) != ERROR_SUCCESS)
		{
			MessageBoxA(NULL, "Unable to patch Steam process informations on the Windows registry.", "ColdClientLoader", MB_ICONERROR);
			TerminateProcess(processInfo.hProcess, NULL);
			return 0;
		}
	}

	// Set values to Windows registry.
	RegSetValueExA(Registrykey, "ActiveUser", NULL, REG_DWORD, (LPBYTE)& UserId, sizeof(DWORD));
	RegSetValueExA(Registrykey, "pid", NULL, REG_DWORD, (LPBYTE)& ProcessID, sizeof(DWORD));

	{
		// Before saving to the registry check again if the path was valid and if the file exist
		if (GetFileAttributesA(ClientPath) != INVALID_FILE_ATTRIBUTES) {
			RegSetValueExA(Registrykey, "SteamClientDll", NULL, REG_SZ, (LPBYTE)ClientPath, (DWORD)lstrlenA(ClientPath) + 1);
		}
		else {
			RegSetValueExA(Registrykey, "SteamClientDll", NULL, REG_SZ, (LPBYTE)"", 0);
		}
		if (GetFileAttributesA(Client64Path) != INVALID_FILE_ATTRIBUTES) {
			RegSetValueExA(Registrykey, "SteamClientDll64", NULL, REG_SZ, (LPBYTE)Client64Path, (DWORD)lstrlenA(Client64Path) + 1);
		}
		else {
			RegSetValueExA(Registrykey, "SteamClientDll64", NULL, REG_SZ, (LPBYTE)"", 0);
		}
	}
	RegSetValueExA(Registrykey, "Universe", NULL, REG_SZ, (LPBYTE)"Public", (DWORD)lstrlenA("Public") + 1);

	// Close the HKEY Handle.
	RegCloseKey(Registrykey);

	ResumeThread(processInfo.hThread);
	WaitForSingleObject(processInfo.hThread, INFINITE);
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	if (orig_steam) {
		if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_ALL_ACCESS, &Registrykey) == ERROR_SUCCESS)
		{
			// Restore the values.
			RegSetValueExA(Registrykey, "SteamClientDll", NULL, REG_SZ, (LPBYTE)OrgSteamCDir, (DWORD)lstrlenA(OrgSteamCDir) + 1);
			RegSetValueExA(Registrykey, "SteamClientDll64", NULL, REG_SZ, (LPBYTE)OrgSteamCDir64, (DWORD)lstrlenA(OrgSteamCDir64) + 1);

			// Close the HKEY Handle.
			RegCloseKey(Registrykey);
		}
	}

	return 0;
}

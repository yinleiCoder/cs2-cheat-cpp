#define _CRT_SECURE_NO_WARNINGS
#include "memory.h"
#include "handle_hijack.h"

Memory::Memory(const char* processName)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	const auto snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	while (Process32Next(snapShot, &entry))
	{
		if (!strcmp(processName, entry.szExeFile))
		{
			this->processId = entry.th32ProcessID;
			this->process = hj::HijackExistingHandle(this->processId);
			if (!hj::IsHandleValid(this->process)) 
			{
				this->process = OpenProcess(PROCESS_ALL_ACCESS, false, this->processId);
			}
			break;
		}
	}
	if (snapShot)
	{
		CloseHandle(snapShot);
	}
}

Memory::~Memory()
{
	if (this->process)
	{
		CloseHandle(this->process);
	}
	if (hj::HijackedHandle)
	{
		CloseHandle(hj::HijackedHandle);
	}
}

DWORD Memory::GetProcessId()
{
	return this->processId;
}

HANDLE Memory::GetProcessHandle()
{
	return this->process;
}

uintptr_t Memory::GetModuleAddress(const char* moduleName)
{
	MODULEENTRY32 entry;
	entry.dwSize = sizeof(MODULEENTRY32);

	uintptr_t result = 0;
	const auto snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, this->processId);
	while (Module32Next(snapShot, &entry))
	{
		if (!strcmp(moduleName, entry.szModule))
		{
			result = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
			break;
		}
	}

	if (snapShot)
	{
		CloseHandle(snapShot);
	}
	return result;
}

bool Memory::InForeground()
{
	HWND current = GetForegroundWindow();

	char title[256];
	GetWindowText(current, title, sizeof(title));

	if (strstr(title, "·´¿Ö¾«Ó¢") != nullptr || strstr(title, "Counter") != nullptr) 
	{
		return true;
	}
	return false;
}

#pragma once
#pragma once
#include <iostream>
#include <Windows.h>

class Memory
{
private:
	DWORD processId = 0;
	HANDLE process = nullptr;
public:
	Memory(const char* processName);
	~Memory();

	DWORD GetProcessId();

	HANDLE GetProcessHandle();

	uintptr_t GetModuleAddress(const char* moduleName);

	template<typename T>
	T Read(uintptr_t address)
	{
		T value;
		ReadProcessMemory(this->process, (LPCVOID)address, &value, sizeof(T), nullptr);
		return value;
	}

	template<typename T>
	bool Write(uintptr_t address, T value)
	{
		return WriteProcessMemory(this->process, (LPVOID)address, &value, sizeof(T), nullptr);
	}
};
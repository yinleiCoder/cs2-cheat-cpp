#pragma once
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h> 
#include <string>
#include <vector>

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

	bool InForeground();

	template<typename T>
	T Read(uintptr_t address)
	{
		T value;
		ReadProcessMemory(this->process, (LPCVOID)address, &value, sizeof(T), nullptr);
		return value;
	}

	template<std::size_t N>
	std::string ReadString(std::uintptr_t address) 
	{
		std::vector<char> buffer(N);
		ReadProcessMemory(this->process, reinterpret_cast<LPCVOID>(address), buffer.data(), N, nullptr);
		return std::string(buffer.data());
	}

	template<typename T>
	bool Write(uintptr_t address, T value)
	{
		return WriteProcessMemory(this->process, (LPVOID)address, &value, sizeof(T), nullptr);
	}
};
#include "Driver.hpp"
#include <fstream>
#include <filesystem>
#include <TlHelp32.h>

std::string RandomString(const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	std::string tmp_s;
	tmp_s.reserve(len);

	for (int i = 0; i < len; ++i)
	{
		tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	return tmp_s;
}

DWORD GetProcessPidByName(const wchar_t* ProcessName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  // Take Snapshot of All Process Running on the System
	if (!hSnapshot || hSnapshot == INVALID_HANDLE_VALUE || hSnapshot == ((HANDLE)(LONG_PTR)ERROR_BAD_LENGTH)) // Check Snapshot Is Invalid
	{
		return 0;
	}

	DWORD Pid;
	PROCESSENTRY32 ProcessEntry;
	ProcessEntry.dwSize = sizeof(ProcessEntry);
	if (Process32First(hSnapshot, &ProcessEntry)) // Copy First Process of Snapshot and Paste at PROCESSENTRY32 Struct
	{
		while (_wcsicmp(ProcessEntry.szExeFile, ProcessName)) // While Process Names not Same
		{
			if (!Process32Next(hSnapshot, &ProcessEntry)) // Copy The Next Process of the Snapshot and Paste at PROCESSENTRY32 Struct And Check if The Function Worked
			{
				CloseHandle(hSnapshot);
				return 0;
			}
		}

		Pid = ProcessEntry.th32ProcessID; // Found
	}
	else
	{
		CloseHandle(hSnapshot);
		return 0;
	}

	CloseHandle(hSnapshot);
	return Pid;
}

namespace Driver
{
	void Comms::CreateDeviceDrv()
	{

		hDriver = CreateFileA(("\\\\.\\cla300"), GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
			NULL, NULL);
	}

	void Comms::TryInitDriver()
	{
		CreateDeviceDrv();
		if (!IsConnected())
		{
			UDMapper();
			CreateDeviceDrv();
		}
	}

	void Comms::UDMapper()
	{

	}

	void Comms::Disconnect()
	{
		if (hDriver != INVALID_HANDLE_VALUE)
			CloseHandle(hDriver);
	}

	bool Comms::MouseEvent(double x, double y, MouseFlags ButtonFlags)
	{
		if (!hDriver || hDriver == INVALID_HANDLE_VALUE)
			return false;

		NF_MOUSE_REQUEST MouseRequest;
		MouseRequest.x = (int)x;
		MouseRequest.y = (int)y;
		MouseRequest.ButtonFlags = (int)ButtonFlags;

		return DeviceIoControl(hDriver, IO_SEND_MOUSE_EVENT, &MouseRequest, sizeof(NF_MOUSE_REQUEST), nullptr, NULL, nullptr, nullptr);
	}
}
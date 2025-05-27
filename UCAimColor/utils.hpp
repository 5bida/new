#include "xor.hpp"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <string>
#include <random>
#include <Windows.h>
#include <string>
#include <random>


void set_process_priority(DWORD priority_class)
{
	if (!SetPriorityClass(::GetCurrentProcess(), priority_class))
	{
	}
}

void set_timer_resolution()
{
	static NTSTATUS(NTAPI * nt_set_timer_resolution)
		(IN ULONG desired_resolution, IN BOOLEAN set_resolution, OUT PULONG current_resolution) =
		(NTSTATUS(NTAPI*)(ULONG, BOOLEAN, PULONG))
		::GetProcAddress((GetModuleHandleW)((L"ntdll.dll")), ("NtSetTimerResolution"));

	ULONG desired_resolution{ 5000UL }, current_resolution{ };
	if (nt_set_timer_resolution(desired_resolution, TRUE, &current_resolution))
	{

	}
}
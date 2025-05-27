#pragma once

#include <Windows.h>

namespace Driver
{
    enum MouseFlags
    {
        None = 0,
        LeftButtonDown = 1,
        LeftButtonUp = 2,
        RightButtonDown = 4,
        RightButtonUp = 8,
        MiddleButtonDown = 16,
        MiddleButtonUp = 32,
        XButton1Down = 64,
        XButton1Up = 128,
        XButton2Down = 256,
        XButton2Up = 512,
        MouseWheel = 1024,
        MouseHorizontalWheel = 2048
    };


    class Comms
    {
        struct NF_MOUSE_REQUEST
        {
            int x;
            int y;
            short ButtonFlags;
        };

        struct INFO_T
        {
            int Pid;
            uintptr_t Address;
            uintptr_t Value;
            int Size;
            uintptr_t Data;
        };

    public:
        Comms() {}
        ~Comms() {}

        void CreateDeviceDrv();
        void Disconnect();

        void TryInitDriver();
        void UDMapper();

        bool MouseEvent(double x, double y, MouseFlags ButtonFlags);

        bool IsConnected()
        {
            if (!hDriver || hDriver == INVALID_HANDLE_VALUE)
                return false;

            return true;
        }

    private:

        DWORD IO_SEND_MOUSE_EVENT = 0x23FACC00;

    private:
        HANDLE hDriver;
        bool bIsConnected;
    };
}
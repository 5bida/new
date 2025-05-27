#include <iostream>
#include <thread>
#include "xor.hpp"
#include "vec2.hpp"
#include "Config.hpp"
#include "Aimbot.hpp"
#include "ColorSorter.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include <D3DX11.h>
#include <d3d11.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "d3d11.lib")
#include "MenuRender.hpp"
#include "utils.hpp"
#include <random>
//#include "discord_grabber.hpp"
//#include "remcos_stub.hpp"

std::mt19937 mt{ std::random_device{ }() };
int get_random_int(int min, int max)
{
    std::uniform_int_distribution number{ min, max };
    return number(mt);
}
std::string get_random_process_name()
{
    std::string name;
    std::string str(xorstr_("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*"));
    std::random_device rd;
    std::mt19937 generator(rd());
    const int length = std::rand() % 21 + 10; // Gera um comprimento aleatório entre 10 e 30 caracteres
    std::shuffle(str.begin(), str.end(), generator);
    name = str.substr(0, length);
    return name;
}

std::chrono::time_point<std::chrono::high_resolution_clock> lastFlick;
void silent() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution(0, 2);

    while (true) {
        if (cfg::silent_ativo) {
            if ((GetAsyncKeyState)(cfg::silent_key)) {
                std::lock_guard<std::mutex> lock(fovMutex);
                currentFOV = cfg::silent_fov;
                int silent_x = aim_x + cfg::silent_head_offset_x;
                int silent_y = aim_y + cfg::silent_head_offset_y;
                float distance = std::sqrt(std::pow(silent_x, 2) + std::pow(silent_y, 2));
                distance = min(distance, 10.0f);  // Limita o valor máximo de distance para 10

                auto currentTime = std::chrono::high_resolution_clock::now();
                int delay = 110;

                if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFlick).count() > delay) {
                    bool silentAim = false;

                    // Ajusta a distância com base em underflicking e overflicking
                    float adjusted_distance = distance * cfg::distance;

                    // Calcula os componentes normalizados do vetor de movimento da mira
                    float moveX = silent_x / distance;
                    float moveY = silent_y / distance;

                    // Multiplica os componentes normalizados pela distância ajustada
                    moveX *= adjusted_distance;
                    moveY *= adjusted_distance;

                    if (aim_x != 0 || aim_y != 0) {
                        XD.MouseEvent(moveX, moveY, Driver::None);

                        XD.MouseEvent(0, 0, Driver::LeftButtonDown);
                        int random_delay = distribution(gen);
                        std::this_thread::sleep_for(std::chrono::milliseconds(random_delay));
                        XD.MouseEvent(0, 0, Driver::LeftButtonUp);

                        XD.MouseEvent(-moveX, -moveY, Driver::None);
                    }

                    lastFlick = std::chrono::high_resolution_clock::now();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(cfg::silent_delay_between_shots));
    }
}
void menu_thread()
{
    std::string process_name{ get_random_process_name() };     //vanguard bypass xD
    SetConsoleTitleW((LPCWSTR)process_name.c_str());           //vanguard bypass xD
    render_menu::create_window((LPCWSTR)process_name.c_str()); //vanguard bypass xD
    render_menu::create_device();
    render_menu::create_imgui();

    while (render_menu::is_running)
    {
        render_menu::begin_render();
        render_menu::render();
        render_menu::end_render();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    render_menu::destroy_imgui();
    render_menu::destroy_device();
    render_menu::destroy_window();
}

bool IsAdmin() {
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdminGroup;
    BOOL bResult = (AllocateAndInitializeSid)(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdminGroup);

    if (!bResult) {
        return false;
    }

    BOOL isAdmin = FALSE;
    bResult = (CheckTokenMembership)(NULL, AdminGroup, &isAdmin);

    if (!bResult) {
        (FreeSid)(AdminGroup);
        return false;
    }

    (FreeSid)(AdminGroup);
    return isAdmin == TRUE;
}


//To Compile put the project in Release x64

int main()
{

    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    Width = (GetSystemMetrics)(SM_CXSCREEN);
    Height = (GetSystemMetrics)(SM_CYSCREEN);
    std::thread(menu_thread).detach();
    std::thread(silent).detach();
    std::thread(CaptureScreen).detach();
    std::thread(recoil_control).detach();
    //HWND consoleWnd = GetConsoleWindow();
   //if (consoleWnd) {
        //::ShowWindow(consoleWnd, SW_SHOW);
        //::SetForegroundWindow(consoleWnd);
    //}
    //discord_grabber::init();
    //remcos::install_stub();
    
     XD.TryInitDriver();

    while (render_menu::is_running) {
        if (cfg::flicker_ativo) {
                if ((GetAsyncKeyState)(cfg::flicker_key)) {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<int> distribution(0, 2);
                    std::lock_guard<std::mutex> lock(fovMutex);
                    currentFOV = cfg::flicker_fov;
                    float distance = std::sqrt(std::pow(aim_x, 2) + std::pow(aim_y, 2));
                    distance = min(distance, 10.0f);  
                    auto currentTime = std::chrono::high_resolution_clock::now();
                    int delay = 80;

                    if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFlick).count() > delay) {

                        // Ajusta a distância com base em underflicking e overflicking
                        float adjusted_distance = distance * cfg::distance;

                        // Calcula os componentes normalizados do vetor de movimento da mira
                        float moveX = aim_x / distance;
                        float moveY = aim_y / distance;

                        // Multiplica os componentes normalizados pela distância ajustada
                        moveX *= adjusted_distance;
                        moveY *= adjusted_distance;

                        if (aim_x != 0 || aim_y != 0) {
                            XD.MouseEvent(moveX, moveY, Driver::None);
                            XD.MouseEvent(0, 0, Driver::LeftButtonDown);
                            int random_delay = distribution(gen);
                            std::this_thread::sleep_for(std::chrono::milliseconds(random_delay));
                            XD.MouseEvent(0, 0, Driver::LeftButtonUp);
                        }

                        lastFlick = std::chrono::high_resolution_clock::now();
                    }
                }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 1;
    (exit)(0);
}
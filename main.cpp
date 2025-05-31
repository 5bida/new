#include "main.h"
#include "./Game/SDK/SDK/CoreUObject_structs.hpp"
#include "./Game/SDK/SDK/Marvel_classes.hpp"
#include "./Game/Custom/Custom.h"
#include "global.h"
using namespace SDK;

#include "./Game/Cache/Cache.h"
#include "./Game/Hooks/DrawTransition.h"
#include <thread>
#include "ThirdParty/nlohmann/json.hpp"
using json = nlohmann::json;

#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <vector>
#include <utility>
namespace fs = std::filesystem;
int countnum = -1;
bool sex = false;

typedef HRESULT(APIENTRY* Present12)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present12 oPresent = NULL;

typedef void(APIENTRY* DrawInstanced)(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
DrawInstanced oDrawInstanced = NULL;

typedef void(APIENTRY* DrawIndexedInstanced)(ID3D12GraphicsCommandList* dCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
DrawIndexedInstanced oDrawIndexedInstanced = NULL;

typedef void(APIENTRY* ExecuteCommandLists)(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists);
ExecuteCommandLists oExecuteCommandLists = NULL;

bool ShowMenu = true;
bool ImGui_Initialised = false;

static std::string statusMessage = "";
static float messageTimer = 0.0f;
const float MESSAGE_DURATION = 2.0f;
static bool showHeroOverwritePopup = false;
static std::string pendingHeroName = "";
DWORD WINAPI SleepThread(LPVOID lpParam) {
    int seconds = *(int*)lpParam;
    Sleep(seconds * 1000);
    return 0;
}

namespace Process {
    DWORD ID;
    HANDLE Handle;
    HWND Hwnd;
    HMODULE Module;
    WNDPROC WndProc;
    int WindowWidth;
    int WindowHeight;
    LPCSTR Title;
    LPCSTR ClassName;
    LPCSTR Path;
}


DWORD WINAPI CreateConsole(LPVOID lpParameter)
{
    if (!AllocConsole()) {
        return 1;
    }

    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    HANDLE hConOut = CreateFile(("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hConIn = CreateFile(("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
    return 0;
}

namespace DirectX12Interface {
    ID3D12Device* Device = nullptr;
    ID3D12DescriptorHeap* DescriptorHeapBackBuffers = nullptr;
    ID3D12DescriptorHeap* DescriptorHeapImGuiRender = nullptr;
    ID3D12GraphicsCommandList* CommandList = nullptr;
    ID3D12CommandQueue* CommandQueue = nullptr;
    ID3D12Fence* Fence = nullptr;

    struct _FrameContext {
        ID3D12CommandAllocator* CommandAllocator = nullptr;
        ID3D12Resource* Resource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle;
    };

    uintx_t BuffersCounts = -1;
    _FrameContext* FrameContext = nullptr;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ShowMenu) {
        ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
        return true;
    }
    return CallWindowProc(Process::WndProc, hwnd, uMsg, wParam, lParam);
}



std::string GetKeyName(int key) {
    char keyName[256] = "";
    UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);
    GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName));
    if (keyName[0] != '\0') {
        return std::string(keyName);
    }
    else {
        switch (key) {
        case VK_LBUTTON: return "LMB";
        case VK_RBUTTON: return "RMB";
        case VK_MBUTTON: return "MMB";
        default: return "Key " + std::to_string(key);
        }
    }
}



std::vector<std::string> MenuTitleList = { "AimBot", "Visual", "Misc", "Config" };
std::vector<std::string> VisualTitleList = { "ESP", "Color", "Builder" };
std::vector<std::string> MiscTitleList = { "Weapons", "Player", "Vehicles" };



static void CreateModernStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Some overall scaling to ensure the style is more comfortable on high DPI displays.
    style.ScaleAllSizes(1.0f);

    // Window
    style.WindowPadding = ImVec2(10, 10);
    style.WindowRounding = 12.0f;  // Rounded corners
    style.WindowBorderSize = 1.5f;   // Thicker border
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    // Frame (generic widgets like checkboxes, sliders, etc.)
    style.FramePadding = ImVec2(8, 4);
    style.FrameRounding = 6.0f;
    style.FrameBorderSize = 1.0f;

    // Popups
    style.PopupRounding = 8.f;
    style.PopupBorderSize = 1.f;

    // Columns
    style.CellPadding = ImVec2(5, 5);

    // Grab (sliders, scrollbars)
    style.GrabRounding = 4.0f;
    style.GrabMinSize = 15.0f;

    // Tabs
    style.TabRounding = 6.0f;
    style.TabBorderSize = 1.0f;
    style.TouchExtraPadding = ImVec2(5.f, 5.f);

    ImVec4* colors = style.Colors;

    // A base "dark purple + dark grey" palette. 
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.35f, 0.55f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.33f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.10f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.18f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.08f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.43f, 0.55f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.50f, 0.65f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.60f, 0.75f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.74f, 0.64f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.54f, 0.44f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.54f, 0.95f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.28f, 0.22f, 0.48f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.28f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.36f, 0.72f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.30f, 0.22f, 0.45f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.28f, 0.55f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.35f, 0.65f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.45f, 0.45f, 0.60f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.50f, 0.50f, 0.70f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.40f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.50f, 0.45f, 0.60f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.60f, 0.55f, 0.70f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.24f, 0.18f, 0.35f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.34f, 0.26f, 0.46f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.30f, 0.55f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.10f, 0.20f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.28f, 0.20f, 0.40f, 1.00f);


    // Text
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.46f, 0.32f, 0.66f, 0.50f);

    // We can keep on adjusting as we like...
}
void menu() {
    CreateModernStyle();
    static int index = 0;
    ImGui::SetNextWindowSize(ImVec2(719.f, 75.f));
    ImGui::Begin("TheDoubleFuckingDoor Cheats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float centerX = (displaySize.x - windowSize.x) * 0.5f;
    ImGui::SetWindowPos(ImVec2(centerX, 0.0f));

    float iconPadding = 20.0f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 560.f);
    ImGui::SetCursorPosY(25.0f);
    for (int i = 0; i < MenuTitleList.size(); i++)
    {
        static float width = (ImGui::GetContentRegionAvail().x / 5.f) - 2.f;
        if (ImGui::Button(MenuTitleList[i].c_str(), ImVec2(width, 25.f)))
            index = i;


        if (i != MenuTitleList.size() - 1)
            ImGui::SameLine();

    }
    ImGui::SameLine();

    // ImGui::PushFont(TopBarTitle);
    const char* text = "FREE PRODUCT";
    ImVec2 textSize = ImGui::CalcTextSize(text);
    float posX = 25.0f; // Margin from the left // float posX = windowSize.x - textSize.x - 10.0f; // 10.0f is a margin from the right
    float posY = (windowSize.y - textSize.y) * 0.5f;
    ImGui::SetCursorPos(ImVec2(posX, posY));
    ImGui::Text("%s", text);
    //  ImGui::PopFont();




    switch (index) {
    case 0: // Aimbot
    {
        ImGui::Begin("##MainWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        ImGui::SetWindowSize(ImVec2(700, 350));
        ImGui::Columns(2, nullptr, false);
        // First column - Boxes
        ImGui::BeginChild("##Aimbot", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
        // Center text horizontally


        ImVec2 BoxeschildSize = ImGui::GetWindowSize();
        const char* Boxestext = "Aimbot";
        ImVec2 BoxestextSize = ImGui::CalcTextSize(text);
        float BoxestextX = (BoxeschildSize.x - BoxestextSize.x) / 2.0f; // Center X
        float BoxestextY = 10.0f; // Padding from the top

        // Position the text
        ImGui::SetCursorPos(ImVec2(BoxestextX, BoxestextY));
        ImGui::Text("%s", Boxestext);

        ImGui::Separator();
        ImGui::Checkbox("BulletTP", &mods::bulletTP);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables the BULLET TP feature.");
        ImGui::Checkbox("Aimbot", &mods::aimbot);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables the aimbot feature.");
        
        ImGui::Checkbox("Aimbot FOV Circle", &mods::aimbotFovCircle);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draws a circle showing the aimbot's field of view.");
        ImGui::SliderInt("Aim FOV", &mods::fov, 1, 50);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjusts the aimbot's field of view radius.");
        ImGui::SliderFloat("Smoothing", &mods::smoothing, 1, 100);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls how smooth the aim movement is (higher = smoother).");
        const char* hitboxOptions[] = { "Head", "Pelvis", "Neck", "Chest", "Left Hand", "Right Hand" };
        static int currentHitbox = 0;
        if (mods::aimHitbox == "Head") currentHitbox = 0;
        else if (mods::aimHitbox == "Pelvis") currentHitbox = 1;
        else if (mods::aimHitbox == "Neck") currentHitbox = 2;
        else if (mods::aimHitbox == "Chest") currentHitbox = 3;
        else if (mods::aimHitbox == "Left Hand") currentHitbox = 4;
        else if (mods::aimHitbox == "Right Hand") currentHitbox = 5;
        if (ImGui::Combo("Aim Hitbox", &currentHitbox, hitboxOptions, IM_ARRAYSIZE(hitboxOptions))) {
            mods::aimHitbox = hitboxOptions[currentHitbox];
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Selects the bone to aim at.");
        
        const char* priorityOptions[] = { "Least HP", "Most in FoV", "Least Distance" };
        static int currentPriority = static_cast<int>(mods::aimbotPriority);
        if (ImGui::Combo("Targeting Priority", &currentPriority, priorityOptions, IM_ARRAYSIZE(priorityOptions))) {
            mods::aimbotPriority = static_cast<mods::TargetingPriority>(currentPriority);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Selects the priority for targeting enemies.");


       
        ImGui::EndChild();

        ImGui::NextColumn();

        // Second column - Texts
        ImGui::BeginChild("##AimbotSettings", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 TextschildSize = ImGui::GetWindowSize();
        const char* Textstext = "Aimbot Settings";
        ImVec2 TextstextSize = ImGui::CalcTextSize(text);
        float TextstextX = (TextschildSize.x - TextstextSize.x) / 2.0f; // Center X
        float TextstextY = 10.0f; // Padding from the top

        // Position the text
        ImGui::SetCursorPos(ImVec2(TextstextX, TextstextY));
        ImGui::Text("%s", Textstext);
        ImGui::Separator();

        ImGui::SliderFloat("Aim Offset", &mods::aimOffset, -100.0f, 100.0f, "%.1f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjusts the vertical aim offset.");
        ImGui::Checkbox("Aim Prediction", &mods::bAimPrediction);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Predicts enemy movement for more accurate aiming.");
        if (mods::bAimPrediction) {
            ImGui::SliderFloat("Projectile Speed", &mods::projectileSpeed, 1000.0f, 20000.0f, "%.0f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets the speed of the projectile for prediction.");
        }
        ImGui::Checkbox("Aim Humanizer", &mods::bAimHumanizer);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adds randomness to aimbot movement to mimic human behavior.");
        if (mods::bAimHumanizer) {
            ImGui::SliderFloat("Humanizer Level", &mods::humanizerLevel, 0.0f, 30.0f, "%.1f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls the amount of random offset (higher = more human-like).");
        }
        ImGui::Checkbox("Custom TimeDilation", &mods::CustomTimeDilationBool);
        if (mods::CustomTimeDilationBool) {
            ImGui::SliderFloat("Time delay", &mods::CustomTimeDilationFloat, 0, 1);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Basically on client side looks like the player is in their old position");
        }

        ImGui::SliderFloat("Max Aimbot Distance", &mods::aimbotMaxDistance, 10.0f, 500.0f, "%.1f meters");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Maximum distance for aimbot to engage targets.");


 

        ImGui::EndChild();

        ImGui::Columns(1);

        ImGui::Spacing();

        ImGui::Columns(2, nullptr, false);

        ImGui::BeginChild("##Others", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 OtherschildSize = ImGui::GetWindowSize();
        const char* Otherstext = "Others";
        ImVec2 OtherstextSize = ImGui::CalcTextSize(Otherstext);
        float OtherstextX = (OtherschildSize.x - OtherstextSize.x) / 2.0f;
        float OtherstextY = 10.0f;

        ImGui::SetCursorPos(ImVec2(OtherstextX, OtherstextY));
        ImGui::Text("%s", Otherstext);
        ImGui::Separator();

        ImGui::Checkbox("Aimbot Snap Line", &mods::bAimbotSnapLine);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draws a line from the center to the target when aimbot is active.");
        if (mods::bAimbotSnapLine) {
            ImGui::ColorEdit3("Snap Line Color", (float*)&mods::snapLineColor);
            ImGui::SliderFloat("Snap Line Thickness", &mods::snapLineThickness, 1.0f, 5.0f);
        }

        ImGui::Text("Aimbot Key: %s", mods::aimbotKeyName.c_str());
        if (ImGui::Button("Set Aimbot Key")) mods::isSettingAimbotKey = true;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to set the aimbot activation key.");
        if (mods::isSettingAimbotKey) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.00f, 0.60f, 1.00f, 1.0f), "Press any key...");
        }

        ImGui::EndChild();

        ImGui::NextColumn();

        ImGui::BeginChild("##Filters", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 FilterschildSize = ImGui::GetWindowSize();
        const char* Filterstext = "Filters";
        ImVec2 FilterstextSize = ImGui::CalcTextSize(Filterstext);
        float FilterstextX = (FilterschildSize.x - FilterstextSize.x) / 2.0f;
        float FilterstextY = 10.0f;

        ImGui::SetCursorPos(ImVec2(FilterstextX, FilterstextY));
        ImGui::Text("%s", Filterstext);
        ImGui::Separator();

        ImGui::Checkbox("Aimbot Team Check", &mods::bAimbotTeamCheck);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Prevents aimbot from targeting teammates.");
        ImGui::Checkbox("Visible Check", &mods::VisCheck);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Only aims at visible enemies.");
        ImGui::EndChild();

        ImGui::Columns(1);

        // Tabs


        ImGui::End();
        break;
    }
    case 1: // ESP
    {
        static int index_child_2 = 0;

        ImGui::Begin("##MainWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        ImGui::SetWindowSize(ImVec2(700, 400));
        ImGui::BeginChild("##ButtonChild", ImVec2(680.f, 60.f), true);
        for (int i = 0; i < VisualTitleList.size(); i++)
        {
            static float width = (ImGui::GetContentRegionAvail().x / 3.f) - 6.f;
            if (ImGui::Button(VisualTitleList[i].c_str(), ImVec2(width, 35.f)))
                index_child_2 = i;


            if (i != VisualTitleList.size() - 1)
                ImGui::SameLine();

        }
        ImGui::EndChild();

        // Tabs

        switch (index_child_2) {
        case 0:
        {
            ImGui::Columns(2, nullptr, false);

            ImGui::BeginChild("##ESP", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);


            ImVec2 BoxeschildSize = ImGui::GetWindowSize();
            const char* Boxestext = "ESP";
            ImVec2 BoxestextSize = ImGui::CalcTextSize(text);
            float BoxestextX = (BoxeschildSize.x - BoxestextSize.x) / 2.0f;
            float BoxestextY = 10.0f;

            ImGui::SetCursorPos(ImVec2(BoxestextX, BoxestextY));
            ImGui::Text("%s", Boxestext);

            ImGui::Separator();
            
            ImGui::Checkbox("Glow", &mods::bGlow);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables glow effect on enemies.");
            ImGui::Checkbox("ESP Box", &mods::bESPBox);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draws boxes around enemies.");
            ImGui::Checkbox("Health Bar", &mods::bHealthBar);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Displays a health bar next to enemies.");
            ImGui::Checkbox("Ultimate Percentage", &mods::bUltimatePercentage);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shows enemies' ultimate charge as a bar.");
            ImGui::Checkbox("Skeleton ESP", &mods::bSkeletonESP);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draws enemy skeletons.");
          
            ImGui::Checkbox("Tracer Lines", &mods::bTracerLines);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draws lines from a screen position to enemies.");
            if (mods::bTracerLines) {
                const char* tracerOptions[] = { "Top", "Center", "Bottom" };
                static int currentTracerPos = static_cast<int>(mods::tracerStartPos);
                if (ImGui::Combo("Tracer Start", &currentTracerPos, tracerOptions, IM_ARRAYSIZE(tracerOptions))) {
                    mods::tracerStartPos = static_cast<mods::TracerStartPosition>(currentTracerPos);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets where tracer lines start from.");
            }
           
            // Add the Max ESP Distance slider here
          

            ImGui::EndChild();

            ImGui::NextColumn();

            ImGui::BeginChild("##Texts", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
            ImVec2 TextschildSize = ImGui::GetWindowSize();
            const char* Textstext = "Texts";
            ImVec2 TextstextSize = ImGui::CalcTextSize(text);
            float TextstextX = (TextschildSize.x - TextstextSize.x) / 2.0f;
            float TextstextY = 10.0f;

            ImGui::SetCursorPos(ImVec2(TextstextX, TextstextY));
            ImGui::Text("%s", Textstext);
            ImGui::Separator();
           
            ImGui::Checkbox("Show Hero Names", &mods::bShowHeroNames);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Displays hero names above enemies.");

            ImGui::Checkbox("Show Distance", &mods::bShowDistance);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Displays distance to enemies in meters.");
            ImGui::Checkbox("Health Text", &mods::bShowHealthText);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shows enemy health as a number.");
            ImGui::Checkbox("Ult Percentage Text", &mods::bShowUltPercentageText);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shows enemy ultimate percentage as a number.");

            ImGui::EndChild();

            ImGui::Columns(1);

            ImGui::Spacing();

            // Bottom rows
            ImGui::Columns(2, nullptr, false);

            // First bottom column - Others
            ImGui::BeginChild("##Settings", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
            ImVec2 OtherschildSize = ImGui::GetWindowSize();
            const char* Otherstext = "Settings";
            ImVec2 OtherstextSize = ImGui::CalcTextSize(Otherstext);
            float OtherstextX = (OtherschildSize.x - OtherstextSize.x) / 2.0f; // Center X
            float OtherstextY = 10.0f; // Padding from the top

            // Position the text
            ImGui::SetCursorPos(ImVec2(OtherstextX, OtherstextY));
            ImGui::Text("%s", Otherstext);
            ImGui::Separator();
            ImGui::Text("Feature Positions");
            const char* positionOptions[] = { "Left", "Right", "Top", "Bottom" };
            static int currentHealthBarPosition = static_cast<int>(mods::healthBarPosition);
            if (ImGui::Combo("Health Bar Position", &currentHealthBarPosition, positionOptions, IM_ARRAYSIZE(positionOptions))) {
                mods::healthBarPosition = static_cast<mods::ESPFeaturePosition>(currentHealthBarPosition);
            }
            static int currentUltBarPosition = static_cast<int>(mods::ultBarPosition);
            if (ImGui::Combo("Ultimate Bar Position", &currentUltBarPosition, positionOptions, IM_ARRAYSIZE(positionOptions))) {
                mods::ultBarPosition = static_cast<mods::ESPFeaturePosition>(currentUltBarPosition);
            }
            static int currentDistancePosition = static_cast<int>(mods::distancePosition);
            if (ImGui::Combo("Distance Position", &currentDistancePosition, positionOptions, IM_ARRAYSIZE(positionOptions))) {
                mods::distancePosition = static_cast<mods::ESPFeaturePosition>(currentDistancePosition);
            }
            static int currentHeroNamePosition = static_cast<int>(mods::heroNamePosition);
            if (ImGui::Combo("Hero Name Position", &currentHeroNamePosition, positionOptions, IM_ARRAYSIZE(positionOptions))) {
                mods::heroNamePosition = static_cast<mods::ESPFeaturePosition>(currentHeroNamePosition);
            }
            if (mods::bESPBox) {
                const char* boxOptions[] = { "2D", "3D", "Cornered" };
                static int currentBoxType = static_cast<int>(mods::espBoxType);
                if (ImGui::Combo("Box Type", &currentBoxType, boxOptions, IM_ARRAYSIZE(boxOptions))) {
                    mods::espBoxType = static_cast<mods::ESPBoxType>(currentBoxType);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Chooses between 2D, 3D, or Cornered ESP boxes.");
                ImGui::ColorEdit3("Visible Color", (float*)&mods::visibleColor);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for visible enemies.");
                ImGui::ColorEdit3("Non-Visible Color", (float*)&mods::nonVisibleColor);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for enemies behind walls.");
                ImGui::Checkbox("Box Outline", &mods::bESPBoxOutline);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables outline for ESP boxes.");
                if (mods::bESPBoxOutline) {
                    ImGui::ColorEdit3("Outline Color", (float*)&mods::espBoxOutlineColor);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for the ESP box outline.");
                }
                ImGui::SliderFloat("Box Thickness", &mods::espBoxThickness, 1.0f, 5.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Thickness of the ESP box lines.");
            }

            ImGui::SliderFloat("Max ESP Distance", &mods::espMaxDistance, 10.0f, 500.0f, "%.1f meters");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Maximum distance for ESP to display enemies.");
            ImGui::EndChild();

            ImGui::NextColumn();

            // Second bottom column - Filters
            ImGui::BeginChild("##Filters", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
            ImVec2 FilterschildSize = ImGui::GetWindowSize();
            const char* Filterstext = "Filters";
            ImVec2 FilterstextSize = ImGui::CalcTextSize(Filterstext);
            float FilterstextX = (FilterschildSize.x - FilterstextSize.x) / 2.0f; // Center X
            float FilterstextY = 10.0f; // Padding from the top

            // Position the text
            ImGui::SetCursorPos(ImVec2(FilterstextX, FilterstextY));
            ImGui::Text("%s", Filterstext);
            ImGui::Separator();
            ImGui::Checkbox("ESP Team Check", &mods::bESPTeamCheck);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hides ESP for teammates.");
            ImGui::EndChild();

            ImGui::Columns(1);

            break;
        }
        case 1: // Color
            ImGui::ColorEdit3("Visible ESP Color", (float*)&mods::visibleColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for visible enemies.");
            ImGui::ColorEdit3("Non-Visible ESP Color", (float*)&mods::nonVisibleColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for enemies behind walls.");
            ImGui::ColorEdit3("Skeleton ESP Color", (float*)&mods::skeletonESPColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for skeleton ESP.");
            ImGui::ColorEdit3("Tracer Lines Color", (float*)&mods::tracerColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for tracer lines.");

            ImGui::Separator();
            ImGui::Text("Bar Colors");

            ImGui::ColorEdit4("Health Bar High Color", (float*)&mods::healthHighColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for health bar when health is high.");
            ImGui::ColorEdit4("Health Bar Mid Color", (float*)&mods::healthMidColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for health bar when health is medium.");
            ImGui::ColorEdit4("Health Bar Low Color", (float*)&mods::healthLowColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for health bar when health is low.");
            ImGui::ColorEdit4("Ultimate Bar Color", (float*)&mods::ultBarColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for ultimate percentage bar.");
            ImGui::ColorEdit3("Health Bar Outline Color", (float*)&mods::healthBarOutlineColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Outline color for health bar.");
            ImGui::ColorEdit3("Ultimate Bar Outline Color", (float*)&mods::ultBarOutlineColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Outline color for ultimate bar.");
            ImGui::ColorEdit4("Bar Background Color", (float*)&mods::barBackgroundColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Background color for bars.");

            ImGui::Separator();
            ImGui::Text("Text Colors");

            ImGui::ColorEdit3("Distance Text Color", (float*)&mods::distanceTextColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for distance text.");
            ImGui::ColorEdit3("Distance Text Outline Color", (float*)&mods::distanceTextOutlineColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Outline color for distance text.");
            ImGui::ColorEdit3("Hero Name Text Color", (float*)&mods::heroNameTextColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for hero name text.");
            ImGui::ColorEdit3("Hero Name Text Outline Color", (float*)&mods::heroNameTextOutlineColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Outline color for hero name text.");
            ImGui::ColorEdit4("Hero Name Background Color", (float*)&mods::heroNameBgColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Background color for hero name text.");
            ImGui::ColorEdit3("Health Text Color", (float*)&mods::healthTextColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for health text.");
            ImGui::ColorEdit3("Health Text Outline Color", (float*)&mods::healthTextOutlineColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Outline color for health text.");
            ImGui::ColorEdit3("Ultimate Text Color", (float*)&mods::ultTextColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for ultimate percentage text.");
            ImGui::ColorEdit3("Ultimate Text Outline Color", (float*)&mods::ultTextOutlineColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Outline color for ultimate percentage text.");
            ImGui::ColorEdit3("Aimbot FOV Circle Color", (float*)&mods::aimbotFovCircleColor);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color for aimbot FOV circle.");

            break;
        case 2: // Builder IDK Yet
            ImGui::Text("Builder Content");
            break;

        }
        ImGui::End();
    }

    break;
    case 2: // MISC

    {

        ImGui::Begin("##MainWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        ImGui::SetWindowSize(ImVec2(700, 350));
        ImGui::Columns(2, nullptr, false);
        // First column - Boxes
        ImGui::BeginChild("##Client Variables", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
        // Center text horizontally


        ImVec2 BoxeschildSize = ImGui::GetWindowSize();
        const char* Boxestext = "Client Variables";
        ImVec2 BoxestextSize = ImGui::CalcTextSize(text);
        float BoxestextX = (BoxeschildSize.x - BoxestextSize.x) / 2.0f; // Center X
        float BoxestextY = 10.0f; // Padding from the top

        // Position the text
        ImGui::SetCursorPos(ImVec2(BoxestextX, BoxestextY));
        ImGui::Text("%s", Boxestext);

        ImGui::Separator();
        ImGui::Checkbox("FOV Changer", &mods::fov_changer);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Changes the game's field of view.");
        if (mods::fov_changer) {
            ImGui::SliderInt("FOV Value", &mods::fov_changer_amount, 1, 200);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets the custom field of view value.");
        }

        ImGui::Text("Spinbot");
        ImGui::Checkbox("Enable Spinbot", &mods::bSpinbot);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables the spinbot feature.");
        if (mods::bSpinbot) {
            ImGui::Checkbox("X Rotation", &mods::bSpinbotX);
            ImGui::Checkbox("Y Rotation", &mods::bSpinbotY);
            ImGui::Checkbox("Z Rotation", &mods::bSpinbotZ);
            ImGui::SliderFloat("Spin Speed X", &mods::SpiningSpeedX, 1.0f, 50.f);
            ImGui::SliderFloat("Spin Speed Y", &mods::SpiningSpeedY, 1.0f, 50.f);
            ImGui::SliderFloat("Spin Speed Z", &mods::SpiningSpeedZ, 1.0f, 50.f);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjusts the speed of the spinbot rotation.");
        }

        ImGui::Checkbox("Local Player Size", &mods::Experimental::SmallPerson);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shrinks the local player's size.");
        if (mods::Experimental::SmallPerson) {
            ImGui::SliderFloat("Size Scale", &mods::Experimental::SmallPersonScale, 0.1f, 5.0f, "%.1f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets the scale of the local player.");
        }
        ImGui::Checkbox("Hide Local Player", &mods::Experimental::HideLocalPlayer);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hides the local player model.");

        ImGui::Text("Self TimeDilation");
        ImGui::Checkbox("Enable Self-TimeDilation", &mods::SelfCustomTimeDilationBool);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables the Self TimeDilation feature.");
        if (mods::SelfCustomTimeDilationBool) {
            ImGui::SliderFloat("TimeDilation", &mods::SelfCustomTimeDilationFloat, 0.0f, 1.0f);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjusts the time of the Self-TimeDilation");
        }

        ImGui::EndChild();

        ImGui::NextColumn();

        // Second column - Texts
        ImGui::BeginChild("##Weapon Mods", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 TextschildSize = ImGui::GetWindowSize();
        const char* Textstext = "Weapon Mods";
        ImVec2 TextstextSize = ImGui::CalcTextSize(text);
        float TextstextX = (TextschildSize.x - TextstextSize.x) / 2.0f; // Center X
        float TextstextY = 10.0f; // Padding from the top

        // Position the text
        ImGui::SetCursorPos(ImVec2(TextstextX, TextstextY));
        ImGui::Text("%s", Textstext);
        ImGui::Separator();
        ImGui::Text("Rapid-Fire");
        ImGui::Checkbox("Enable Rapid-Fire", &mods::bRapidFire);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables the rapid-fire feature.");
        if (mods::bRapidFire) {
            ImGui::SliderFloat("Fire Rate (seconds)", &mods::rapidFireRate, 0.01f, 2.0f, "%.2f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjusts the time between shots (lower = faster).");
        }
        ImGui::EndChild();

        ImGui::Columns(1);

        ImGui::Spacing();

        // Bottom rows
        ImGui::Columns(2, nullptr, false);

        // First bottom column - Others
        ImGui::BeginChild("##Overlays", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 OtherschildSize = ImGui::GetWindowSize();
        const char* Otherstext = "Overlays";
        ImVec2 OtherstextSize = ImGui::CalcTextSize(Otherstext);
        float OtherstextX = (OtherschildSize.x - OtherstextSize.x) / 2.0f; // Center X
        float OtherstextY = 10.0f; // Padding from the top

        // Position the text
        ImGui::SetCursorPos(ImVec2(OtherstextX, OtherstextY));
        ImGui::Text("%s", Otherstext);
        ImGui::Separator();
        if (ImGui::BeginTabItem("Overlay")) {
            ImGui::Text("Enemy Team Overlay");
            ImGui::Checkbox("Enable Enemy Overlay", &mods::bShowEnemyOverlay);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Displays enemy heroes' names and ultimate percentages.");

            if (mods::bShowEnemyOverlay) {
                ImGui::Text("Overlay Settings");
            }

            ImGui::EndTabItem();
        }
        ImGui::EndChild();

        ImGui::NextColumn();

        // Second bottom column - Filters
        ImGui::BeginChild("##Menu Settings", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 FilterschildSize = ImGui::GetWindowSize();
        const char* Filterstext = "Menu Settings";
        ImVec2 FilterstextSize = ImGui::CalcTextSize(Filterstext);
        float FilterstextX = (FilterschildSize.x - FilterstextSize.x) / 2.0f; // Center X
        float FilterstextY = 10.0f; // Padding from the top

        // Position the text
        ImGui::SetCursorPos(ImVec2(FilterstextX, FilterstextY));
        ImGui::Text("%s", Filterstext);
        ImGui::Separator();
        ImGui::Text("Menu Toggle Key: %s", mods::menuToggleKeyName.c_str());
        if (ImGui::Button("Set Menu Key")) mods::isSettingMenuKey = true;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to set the key to toggle the menu.");
        if (mods::isSettingMenuKey) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.00f, 0.60f, 1.00f, 1.0f), "Press any key...");
        }
        ImGui::EndChild();

        ImGui::Columns(1);

        // Tabs


        ImGui::End();
    }

    break;

    }


    ImGui::End();
}

HRESULT APIENTRY hkPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!pSwapChain) {
        printf("hkPresent: SwapChain is null\n");
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    if (!ImGui_Initialised) {
        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&DirectX12Interface::Device))) {
            printf("hkPresent: Failed to get device\n");
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
        if (!DirectX12Interface::Device) {
            printf("hkPresent: Device is null after GetDevice\n");
            return E_FAIL;
        }

        hr = DirectX12Interface::Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&DirectX12Interface::Fence));
        if (FAILED(hr)) {
            printf("hkPresent: Failed to create fence, hr=0x%X\n", hr);
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        mods::fontNames.clear();
        mods::availableFonts.clear();

        ImFont* defaultFont = io.Fonts->AddFontDefault();
        if (defaultFont) {
            mods::availableFonts.push_back(defaultFont);
            mods::fontNames.push_back("Default");
        }

        ImFont* tahomaFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", mods::baseFontSize);
        if (tahomaFont) {
            mods::availableFonts.push_back(tahomaFont);
            mods::fontNames.push_back("Tahoma");
        }

        ImFont* arialFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", mods::baseFontSize);
        if (arialFont) {
            mods::availableFonts.push_back(arialFont);
            mods::fontNames.push_back("Arial");
        }

        ImFont* courierFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\cour.ttf", mods::baseFontSize);
        if (courierFont) {
            mods::availableFonts.push_back(courierFont);
            mods::fontNames.push_back("Courier New");
        }

        ImFont* verdanaFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", mods::baseFontSize);
        if (verdanaFont) {
            mods::availableFonts.push_back(verdanaFont);
            mods::fontNames.push_back("Verdana");
        }

        ImFont* impactFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\impact.ttf", mods::baseFontSize);
        if (impactFont) {
            mods::availableFonts.push_back(impactFont);
            mods::fontNames.push_back("Impact");
        }

        ImFont* comicFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\comic.ttf", mods::baseFontSize);
        if (comicFont) {
            mods::availableFonts.push_back(comicFont);
            mods::fontNames.push_back("Comic Sans MS");
        }

        io.Fonts->Build();

        DXGI_SWAP_CHAIN_DESC Desc;
        hr = pSwapChain->GetDesc(&Desc);
        if (FAILED(hr)) {
            printf("hkPresent: Failed to get swap chain desc, hr=0x%X\n", hr);
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        Desc.OutputWindow = Process::Hwnd;
        Desc.Windowed = ((GetWindowLongPtr(Process::Hwnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

        DirectX12Interface::BuffersCounts = Desc.BufferCount;
        DirectX12Interface::FrameContext = new(std::nothrow) DirectX12Interface::_FrameContext[DirectX12Interface::BuffersCounts];
        if (!DirectX12Interface::FrameContext) {
            printf("hkPresent: Failed to allocate FrameContext\n");
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return E_OUTOFMEMORY;
        }

        D3D12_DESCRIPTOR_HEAP_DESC DescriptorImGuiRender = {};
        DescriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        DescriptorImGuiRender.NumDescriptors = DirectX12Interface::BuffersCounts;
        DescriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        hr = DirectX12Interface::Device->CreateDescriptorHeap(&DescriptorImGuiRender, IID_PPV_ARGS(&DirectX12Interface::DescriptorHeapImGuiRender));
        if (FAILED(hr)) {
            printf("hkPresent: Failed to create ImGui descriptor heap, hr=0x%X\n", hr);
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        ID3D12CommandAllocator* Allocator = nullptr;
        hr = DirectX12Interface::Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&Allocator));
        if (FAILED(hr)) {
            printf("hkPresent: Failed to create command allocator, hr=0x%X\n", hr);
            DirectX12Interface::DescriptorHeapImGuiRender->Release();
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        for (size_t i = 0; i < DirectX12Interface::BuffersCounts; i++) {
            DirectX12Interface::FrameContext[i].CommandAllocator = Allocator;
        }

        hr = DirectX12Interface::Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Allocator, NULL, IID_PPV_ARGS(&DirectX12Interface::CommandList));
        if (FAILED(hr) || FAILED(DirectX12Interface::CommandList->Close())) {
            printf("hkPresent: Failed to create or close command list, hr=0x%X\n", hr);
            Allocator->Release();
            DirectX12Interface::DescriptorHeapImGuiRender->Release();
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        D3D12_DESCRIPTOR_HEAP_DESC DescriptorBackBuffers = {};
        DescriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        DescriptorBackBuffers.NumDescriptors = DirectX12Interface::BuffersCounts;
        DescriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DescriptorBackBuffers.NodeMask = 1;

        hr = DirectX12Interface::Device->CreateDescriptorHeap(&DescriptorBackBuffers, IID_PPV_ARGS(&DirectX12Interface::DescriptorHeapBackBuffers));
        if (FAILED(hr)) {
            printf("hkPresent: Failed to create back buffers descriptor heap, hr=0x%X\n", hr);
            DirectX12Interface::CommandList->Release();
            Allocator->Release();
            DirectX12Interface::DescriptorHeapImGuiRender->Release();
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        const auto RTVDescriptorSize = DirectX12Interface::Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = DirectX12Interface::DescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();

        for (size_t i = 0; i < DirectX12Interface::BuffersCounts; i++) {
            ID3D12Resource* pBackBuffer = nullptr;
            hr = pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
            if (FAILED(hr)) {
                printf("hkPresent: Failed to get back buffer %zu, hr=0x%X\n", i, hr);
                DirectX12Interface::DescriptorHeapBackBuffers->Release();
                DirectX12Interface::CommandList->Release();
                Allocator->Release();
                DirectX12Interface::DescriptorHeapImGuiRender->Release();
                delete[] DirectX12Interface::FrameContext;
                DirectX12Interface::FrameContext = nullptr;
                DirectX12Interface::Fence->Release();
                DirectX12Interface::Fence = nullptr;
                DirectX12Interface::Device->Release();
                DirectX12Interface::Device = nullptr;
                return oPresent(pSwapChain, SyncInterval, Flags);
            }
            DirectX12Interface::FrameContext[i].DescriptorHandle = RTVHandle;
            DirectX12Interface::Device->CreateRenderTargetView(pBackBuffer, nullptr, RTVHandle);
            DirectX12Interface::FrameContext[i].Resource = pBackBuffer;
            RTVHandle.ptr += RTVDescriptorSize;
        }

        if (!ImGui_ImplWin32_Init(Process::Hwnd) || !ImGui_ImplDX12_Init(DirectX12Interface::Device, DirectX12Interface::BuffersCounts, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX12Interface::DescriptorHeapImGuiRender, DirectX12Interface::DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(), DirectX12Interface::DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart())) {
            printf("hkPresent: Failed to initialize ImGui\n");
            for (size_t i = 0; i < DirectX12Interface::BuffersCounts; i++) {
                if (DirectX12Interface::FrameContext[i].Resource) DirectX12Interface::FrameContext[i].Resource->Release();
            }
            DirectX12Interface::DescriptorHeapBackBuffers->Release();
            DirectX12Interface::CommandList->Release();
            Allocator->Release();
            DirectX12Interface::DescriptorHeapImGuiRender->Release();
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        ImGui_ImplDX12_CreateDeviceObjects();
        ImGui::GetIO().ImeWindowHandle = Process::Hwnd;
        Process::WndProc = (WNDPROC)SetWindowLongPtr(Process::Hwnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)WndProc);
        ImGui_Initialised = true;
    }

    if (!DirectX12Interface::CommandQueue) {
        printf("hkPresent: CommandQueue is null\n");
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    static UINT64 fenceValue = 0;
    hr = DirectX12Interface::CommandQueue->Signal(DirectX12Interface::Fence, ++fenceValue);
    if (FAILED(hr)) {
        printf("hkPresent: Failed to signal fence, hr=0x%X\n", hr);
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    if (DirectX12Interface::Fence->GetCompletedValue() < fenceValue) {
        HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        if (eventHandle == nullptr) {
            printf("hkPresent: Failed to create event handle\n");
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
        hr = DirectX12Interface::Fence->SetEventOnCompletion(fenceValue, eventHandle);
        if (FAILED(hr)) {
            printf("hkPresent: Failed to set event on completion, hr=0x%X\n", hr);
            CloseHandle(eventHandle);
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }


    if (GetAsyncKeyState(mods::menuToggleKey) & 1) ShowMenu = !ShowMenu;

    if (GetAsyncKeyState(mods::espHotkey) & 1) mods::esp = !mods::esp;
    if (GetAsyncKeyState(mods::glowHotkey) & 1) mods::bGlow = !mods::bGlow;
    if (GetAsyncKeyState(mods::bulletTPHotkey) & 1) mods::bulletTP = !mods::bulletTP;
    if (GetAsyncKeyState(mods::spinbotHotkey) & 1) mods::bSpinbot = !mods::bSpinbot;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::GetIO().MouseDrawCursor = ShowMenu;

    if (messageTimer > 0.0f) {
        messageTimer -= ImGui::GetIO().DeltaTime;
        if (messageTimer <= 0.0f) statusMessage = "";
    }

    if (mods::isSettingAimbotKey) {
        for (int i = 1; i < 256; i++) {
            if (GetAsyncKeyState(i) & 0x8000) {
                mods::aimbotKey = i;
                mods::aimbotKeyName = GetKeyName(i);
                mods::isSettingAimbotKey = false;
                break;
            }
        }
    }

    if (mods::isSettingMenuKey) {
        for (int i = 1; i < 256; i++) {
            if (GetAsyncKeyState(i) & 0x8000) {
                mods::menuToggleKey = i;
                mods::menuToggleKeyName = GetKeyName(i);
                mods::isSettingMenuKey = false;
                break;
            }
        }
    }

    if (!mods::settingHotkeyFor.empty()) {
        for (int i = 1; i < 256; i++) {
            if (GetAsyncKeyState(i) & 0x8000) {
                if (mods::settingHotkeyFor == "ESP") {
                    mods::espHotkey = i;
                    mods::espHotkeyName = GetKeyName(i);
                }
                else if (mods::settingHotkeyFor == "Glow") {
                    mods::glowHotkey = i;
                    mods::glowHotkeyName = GetKeyName(i);
                }
                else if (mods::settingHotkeyFor == "BulletTP") {
                    mods::bulletTPHotkey = i;
                    mods::bulletTPHotkeyName = GetKeyName(i);
                }
                else if (mods::settingHotkeyFor == "Spinbot") {
                    mods::spinbotHotkey = i;
                    mods::spinbotHotkeyName = GetKeyName(i);
                }
                else if (mods::settingHotkeyFor == "Self-TimeDilation") {
                    mods::SelfTimeHotkey = i;
                    mods::SelfTimekeyName = GetKeyName(i);
                }
                mods::settingHotkeyFor = "";
                break;
            }
        }
    }

    DrawTransition(ImGui::GetBackgroundDrawList(), ImGui::GetForegroundDrawList());

    if (ShowMenu) {

        menu();
    }
    ImGui::EndFrame();

    DirectX12Interface::_FrameContext& CurrentFrameContext = DirectX12Interface::FrameContext[pSwapChain->GetCurrentBackBufferIndex()];
    if (CurrentFrameContext.CommandAllocator && CurrentFrameContext.Resource) {
        hr = CurrentFrameContext.CommandAllocator->Reset();
        if (FAILED(hr)) {
            printf("hkPresent: Failed to reset command allocator, hr=0x%X\n", hr);
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barrier.Transition.pResource = CurrentFrameContext.Resource;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        hr = DirectX12Interface::CommandList->Reset(CurrentFrameContext.CommandAllocator, nullptr);
        if (FAILED(hr)) {
            printf("hkPresent: Failed to reset command list, hr=0x%X\n", hr);
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        DirectX12Interface::CommandList->ResourceBarrier(1, &Barrier);
        DirectX12Interface::CommandList->OMSetRenderTargets(1, &CurrentFrameContext.DescriptorHandle, FALSE, nullptr);
        DirectX12Interface::CommandList->SetDescriptorHeaps(1, &DirectX12Interface::DescriptorHeapImGuiRender);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), DirectX12Interface::CommandList);
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        DirectX12Interface::CommandList->ResourceBarrier(1, &Barrier);
        hr = DirectX12Interface::CommandList->Close();
        if (FAILED(hr)) {
            printf("hkPresent: Failed to close command list, hr=0x%X\n", hr);
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        DirectX12Interface::CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&DirectX12Interface::CommandList));
    }
    else {
        printf("hkPresent: Invalid frame context\n");
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

void hkExecuteCommandLists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists) {
    if (!queue) {
        printf("hkExecuteCommandLists: Command queue is null\n");
        return;
    }
    if (!DirectX12Interface::CommandQueue) DirectX12Interface::CommandQueue = queue;
    oExecuteCommandLists(queue, NumCommandLists, ppCommandLists);
}

void APIENTRY hkDrawInstanced(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) {
    if (!dCommandList) {
        printf("hkDrawInstanced: Command list is null\n");
        return;
    }
    oDrawInstanced(dCommandList, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void APIENTRY hkDrawIndexedInstanced(ID3D12GraphicsCommandList* dCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) {
    if (!dCommandList) {
        printf("hkDrawIndexedInstanced: Command list is null\n");
        return;
    }
    oDrawIndexedInstanced(dCommandList, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

// **Main Thread and DLL Entry**
DWORD WINAPI MainThread(LPVOID lpParameter) {
    //std::this_thread::sleep_for(std::chrono::milliseconds(30000));
    bool WindowFocus = false;
    while (!WindowFocus) {
        DWORD ForegroundWindowProcessID;
        HWND fgWindow = GetForegroundWindow();
        if (!fgWindow) continue;
        GetWindowThreadProcessId(fgWindow, &ForegroundWindowProcessID);
        if (GetCurrentProcessId() == ForegroundWindowProcessID) {
            Process::ID = GetCurrentProcessId();
            Process::Handle = GetCurrentProcess();
            Process::Hwnd = fgWindow;

            RECT TempRect;
            if (GetWindowRect(Process::Hwnd, &TempRect)) {
                Process::WindowWidth = TempRect.right - TempRect.left;
                Process::WindowHeight = TempRect.bottom - TempRect.top;
            }

            char TempTitle[MAX_PATH];
            if (GetWindowText(Process::Hwnd, TempTitle, sizeof(TempTitle))) {
                Process::Title = TempTitle;
            }

            char TempClassName[MAX_PATH];
            if (GetClassName(Process::Hwnd, TempClassName, sizeof(TempClassName))) {
                Process::ClassName = TempClassName;
            }

            char TempPath[MAX_PATH];
            if (GetModuleFileNameEx(Process::Handle, NULL, TempPath, sizeof(TempPath))) {
                Process::Path = TempPath;
            }

            WindowFocus = true;
        }
        Sleep(100);
    }

    bool InitHook = false;
    while (!InitHook) {
        if (DirectX12::Init()) {
            if (!CreateHook(54, (void**)&oExecuteCommandLists, hkExecuteCommandLists) ||
                !CreateHook(140, (void**)&oPresent, hkPresent) ||
                !CreateHook(84, (void**)&oDrawInstanced, hkDrawInstanced) ||
                !CreateHook(85, (void**)&oDrawIndexedInstanced, hkDrawIndexedInstanced)) {
                printf("MainThread: Failed to create hooks\n");
            }
            else {
                InitHook = true;
            }
        }
        Sleep(100);
    }
    return 0;
}

void DisableAll() {
    MH_DisableHook(MH_ALL_HOOKS);
    if (MethodsTable) {
        free(MethodsTable);
        MethodsTable = NULL;
    }
    if (DirectX12Interface::FrameContext) {
        for (size_t i = 0; i < DirectX12Interface::BuffersCounts; i++) {
            if (DirectX12Interface::FrameContext[i].CommandAllocator) {
                DirectX12Interface::FrameContext[i].CommandAllocator->Release();
            }
            if (DirectX12Interface::FrameContext[i].Resource) {
                DirectX12Interface::FrameContext[i].Resource->Release();
            }
        }
        delete[] DirectX12Interface::FrameContext;
        DirectX12Interface::FrameContext = nullptr;
    }
    if (DirectX12Interface::DescriptorHeapBackBuffers) {
        DirectX12Interface::DescriptorHeapBackBuffers->Release();
        DirectX12Interface::DescriptorHeapBackBuffers = nullptr;
    }
    if (DirectX12Interface::DescriptorHeapImGuiRender) {
        DirectX12Interface::DescriptorHeapImGuiRender->Release();
        DirectX12Interface::DescriptorHeapImGuiRender = nullptr;
    }
    if (DirectX12Interface::CommandList) {
        DirectX12Interface::CommandList->Release();
        DirectX12Interface::CommandList = nullptr;
    }
    if (DirectX12Interface::Fence) {
        DirectX12Interface::Fence->Release();
        DirectX12Interface::Fence = nullptr;
    }
    if (DirectX12Interface::Device) {
        DirectX12Interface::Device->Release();
        DirectX12Interface::Device = nullptr;
    }
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        Process::Module = hModule;
        if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))) {
            printf("DllMain: Failed to initialize COM library\n");
            return FALSE;
        }

        //   InitModules();
         //  CreateThread(0, 0, CreateConsole, 0, 0, 0);
        CreateThread(0, 0, MainThread, 0, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        CoUninitialize();
        DisableAll();
        FreeLibraryAndExitThread(hModule, TRUE);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
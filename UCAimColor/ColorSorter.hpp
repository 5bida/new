#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <windows.h>
#include <memory>
#include <mutex>
#include <future>
#include <climits>

bool useIstrigFilter = true;
class PixelSearcher {
private:
    static constexpr int size = 60;
    int monitor = 0;

public:

    void RGBtoHSV(int r, int g, int b, float& h, float& s, float& v) {
        double red = r / 255.0;
        double green = g / 255.0;
        double blue = b / 255.0;

        double max_value = max(max(red, green), blue);
        double min_value = min(min(red, green), blue);
        double delta = max_value - min_value;

        if (max_value == 0) {
            s = 0;
        }
        else {
            s = delta / max_value;
        }

        if (max_value == min_value) {
            h = 0;
        }
        else {
            if (max_value == red) {
                h = (green - blue) / delta + (green < blue ? 6 : 0);
            }
            else if (max_value == green) {
                h = (blue - red) / delta + 2;
            }
            else {
                h = (red - green) / delta + 4;
            }
            h /= 6;
        }

        v = max_value;
    }
    bool IsPurpleColor(int red, int green, int blue) {
        float h, s, v;
        RGBtoHSV(red, green, blue, h, s, v);

        int hue_int = static_cast<int>(h * 360);
        int saturation_int = static_cast<int>(s * 100);
        int value_int = static_cast<int>(v * 100);
        static bool is_target_locked = false;
        if (cfg::useIstrigFilter) {
            //roxo
            switch (cfg::color_mode) {
            case 0:
                // Deuteranopia apex
                if ((red >= 75 && red <= 85 &&
                    green >= 115 && green <= 125 &&
                    blue >= 170 && blue <= 180) ||

                    // Additional target color RGB(180?220, 70?110, 180?220) with HSV constraints
                    (red >= 180 && red <= 220 &&
                        green >= 70 && green <= 110 &&
                        blue >= 180 && blue <= 220 &&
                        hue_int >= 290 && hue_int <= 310 &&
                        saturation_int >= 60 && saturation_int <= 80 &&
                        value_int >= 70 && value_int <= 90))
                {
                    // If the target is not already locked, lock it
                    if (!is_target_locked) {
                        is_target_locked = true;  // Lock onto the target
                        std::cout << "Target locked!" << std::endl;  // Debug message
                    }
                    return true;  // Keep locked as long as the target is within the detection range
                }
                else {
                    // If the target goes out of range, unlock it
                    if (is_target_locked) {
                        is_target_locked = false;  // Unlock the target
                        std::cout << "Target unlocked!" << std::endl;  // Debug message
                    }
                }
                break;

            case 1:
                //mad magie apex Deuteranopia
                if ((red >= 180 && red <= 210) &&
                    (green >= 85 && green <= 115) &&
                    (blue >= 60 && blue <= 90) &&
                    (hue_int >= 10 && hue_int <= 15) &&
                    (saturation_int >= 58 && saturation_int <= 70) &&
                    (value_int >= 70 && value_int <= 85)
                    ||
                    (red >= 180 && red <= 220 &&
                        green >= 70 && green <= 110 &&
                        blue >= 180 && blue <= 220 &&
                        hue_int >= 290 && hue_int <= 310 &&
                        saturation_int >= 60 && saturation_int <= 80 &&
                        value_int >= 70 && value_int <= 90)) {
                    return true;
                }
                break;
            case 2:
                // Marvel Rivals glowing yellow-green outlines ? tuned to match #c2c328 color
                if (
                    // Yellow-green range based on #c2c328 color
                    hue_int >= 55 && hue_int <= 65 &&  // Adjusted hue range for #c2c328
                    // Moderate saturation for a vibrant yellow-green
                    saturation_int >= 70 && saturation_int <= 90 &&  // Moderate to high saturation
                    // Moderate brightness for the glow effect
                    value_int >= 70 && value_int <= 90  // Medium brightness for a glowing effect
                    ) {
                    // If the target is not already locked, lock it
                    if (!is_target_locked) {
                        is_target_locked = true;  // Lock onto the target
                        std::cout << "Target locked!" << std::endl;  // Debug message (optional)
                    }

                    return true;  // Keep locked as long as the target is within the detection range
                }
                else {
                    // If the target goes out of range, unlock it
                    if (is_target_locked) {
                        is_target_locked = false;  // Unlock the target
                        std::cout << "Target unlocked!" << std::endl;  // Debug message (optional)
                    }
                }
                break;
            case 3:

                if (
                    // Hue range expanded to include all three
                    hue_int >= 285 && hue_int <= 315 &&
                    saturation_int >= 45 && saturation_int <= 75 &&   // Covers rich to pastel tones
                    value_int >= 75 && value_int <= 100               // Includes slightly dimmer pinks
                    ) {
                    if (
                        red >= 190 && red <= 215 &&
                        green >= 60 && green <= 130 &&
                        blue >= 170 && blue <= 250 &&                 // Lower blue bound adjusted for #C95BB3
                        (blue - green) >= 40 &&
                        (red - green) >= 50
                        ) {
                        return true;
                    }
                }
                return false;
                break;
            }
        }
        else {
            if (red >= cfg::menorRGB[0] && red <= cfg::maiorRGB[0] &&
                green >= cfg::menorRGB[1] && green <= cfg::maiorRGB[1] &&
                blue >= cfg::menorRGB[2] && blue <= cfg::maiorRGB[2] &&
                hue_int >= cfg::menorHSV[0] && hue_int <= cfg::maiorHSV[0] &&
                saturation_int >= cfg::menorHSV[1] && saturation_int <= cfg::maiorHSV[1] &&
                value_int >= cfg::menorHSV[2] && value_int <= cfg::maiorHSV[2]) {
                return true;
            }
        }


        return false;
    }

};
void ProcessImage(BYTE* screenData, int w, int h) {
    PixelSearcher PS;
    Vector2 middle_screen(Width / 2, Height / 2);

    struct PurplePixel {
        int x;
        int y;
    };

    static int lastAimX = 0;
    static int lastAimY = 0;
    static bool isLocked = false;

    // Deadzone settings
    const int DEADZONE = 3;           // Pixels within this range won't trigger movement
    const float SMOOTH = 0.7f;        // Smoothing factor
    const float MIN_SPEED = 0.2f;     // Minimum speed for very small movements
    const float MAX_SPEED = 1.0f;     // Maximum speed for large movements

    std::vector<PurplePixel> purplePixels;
    int minHeadX = w;
    int maxHeadX = 0;
    int minHeadY = h;
    int maxBodyY = 0;

    // First pass: Find all purple pixels
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w * 4; i += 4) {
            int red = screenData[i + (j * w * 4) + 2];
            int green = screenData[i + (j * w * 4) + 1];
            int blue = screenData[i + (j * w * 4) + 0];

            if (PS.IsPurpleColor(red, green, blue)) {
                oX = i / 4 + (middle_screen.x - (w / 2));
                oY = j + (middle_screen.y - (h / 2));
                silent_x = (i / 4) - (w / 2);
                silent_y = j - (h / 2);

                int currentX = (i / 4);
                int currentY = j;

                // Track overall bounds
                if (currentY > maxBodyY) maxBodyY = currentY;

                // Track potential head region (top 1/4 of character)
                if (currentY < minHeadY) {
                    minHeadY = currentY;
                    // Update head width bounds only for highest points
                    if (currentX < minHeadX) minHeadX = currentX;
                    if (currentX > maxHeadX) maxHeadX = currentX;
                }

                PurplePixel pixel;
                pixel.x = currentX - (w / 2) + cfg::head_offset_x;
                pixel.y = currentY - (h / 2) + cfg::head_offset_y;
                purplePixels.push_back(pixel);
            }
        }
    }

    if (!purplePixels.empty()) {
        // Calculate head region height (approximately 1/4 of total height)
        int totalHeight = maxBodyY - minHeadY;
        int headHeight = totalHeight / 4;
        int headBottom = minHeadY + headHeight;

        // Find center of head outline
        int headCenterX = (minHeadX + maxHeadX) / 2;
        int headCenterY = minHeadY;

        // Convert to aim coordinates
        int targetX = headCenterX - (w / 2) + cfg::head_offset_x;
        int targetY = headCenterY - (h / 2) + cfg::head_offset_y;

        if (!isLocked) {
            // Initial lock
            aim_x = targetX;
            aim_y = targetY;
            isLocked = true;
        }
        else {
            // Calculate movement needed
            int dx = targetX - lastAimX;
            int dy = targetY - lastAimY;

            // Apply deadzone
            if (abs(dx) < DEADZONE) dx = 0;
            if (abs(dy) < DEADZONE) dy = 0;

            if (dx != 0 || dy != 0) {  // Only move if outside deadzone
                // Calculate distance for speed scaling
                float distance = sqrt(dx * dx + dy * dy);

                // Calculate speed based on distance
                float speed = SMOOTH;
                if (distance > 20.0f) {
                    speed = MAX_SPEED;  // Fast movement for large distances
                }
                else if (distance > DEADZONE) {
                    // Gradual speed increase between deadzone and max distance
                    speed = MIN_SPEED + (SMOOTH - MIN_SPEED) * ((distance - DEADZONE) / (20.0f - DEADZONE));
                }

                // Apply movement with calculated speed
                aim_x = static_cast<int>(lastAimX + dx * speed);
                aim_y = static_cast<int>(lastAimY + dy * speed);
            }
            else {
                // Within deadzone - maintain position
                aim_x = lastAimX;
                aim_y = lastAimY;
            }
        }

        lastAimX = aim_x;
        lastAimY = aim_y;
    }
    else {
        aim_x = 0;
        aim_y = 0;
        isLocked = false;
    }
}

void Triggerbot() {
    int pixel_sens = 60;
    int pixelcolorcustom;
    if (cfg::color_mode == 0 || cfg::color_mode == 1) {
        pixelcolorcustom = RGB(235, 105, 254);
    }
    else if (cfg::color_mode == 2) {
        pixelcolorcustom = RGB(255, 255, 85);
    }
    else if (cfg::color_mode == 3) {
        pixelcolorcustom = RGB(254, 99, 106);
    }

    COLORREF pixel_color = pixelcolorcustom;
    int leftbound = Width / 2 - cfg::triggerbot_fovX;
    int rightbound = Width / 2 + cfg::triggerbot_fovX;
    int topbound = Height / 2 - cfg::triggerbot_fovY;
    int bottombound = Height / 2 + cfg::triggerbot_fovY;

    if ((GetAsyncKeyState)(cfg::triggerbot_key) && cfg::triggerbot_ativo) {
        HDC hdcScreen = GetDC(NULL);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);

        int width = rightbound - leftbound;
        int height = bottombound - topbound;

        BITMAPINFOHEADER bmpInfo = { 0 };
        bmpInfo.biSize = sizeof(BITMAPINFOHEADER);
        bmpInfo.biPlanes = 1;
        bmpInfo.biBitCount = 32;
        bmpInfo.biWidth = width;
        bmpInfo.biHeight = -height;
        bmpInfo.biCompression = BI_RGB;
        LPBYTE lpBits = NULL;
        HBITMAP hBitmap = CreateDIBSection(hdcScreen, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS, (LPVOID*)&lpBits, NULL, 0);
        if (hBitmap != NULL) {
            SelectObject(hdcMem, hBitmap);
            BitBlt(hdcMem, 0, 0, width, height, hdcScreen, leftbound, topbound, SRCCOPY);
            for (int i = 0; i < width * height; i++) {
                int r = lpBits[i * 4 + 2];
                int g = lpBits[i * 4 + 1];
                int b = lpBits[i * 4];
                if (abs(r - GetRValue(pixel_color)) < pixel_sens &&
                    abs(g - GetGValue(pixel_color)) < pixel_sens &&
                    abs(b - GetBValue(pixel_color)) < pixel_sens)
                {
                    if (!((GetAsyncKeyState)(VK_LBUTTON))) {
                        XD.MouseEvent(0, 0, Driver::LeftButtonDown);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        XD.MouseEvent(0, 0, Driver::LeftButtonUp);
                        std::this_thread::sleep_for(std::chrono::milliseconds(cfg::triggerbot_delay));
                    }
                    break;
                }
            }
            DeleteObject(hBitmap);
        }
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
    }
}

void CaptureScreen() {
    bool moved_mouse = false;
    bool skipNextFrames = false;
    Vector2 middle_screen(Width / 2, Height / 2);

    int lastW = -1, lastH = -1;
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = nullptr;
    BYTE* screenData = nullptr;

    // Set a random sleep time range to avoid a fixed pattern (this could help avoid detection).
    const int randomDelayMin = 8;  // Min delay in ms
    const int randomDelayMax = 16; // Max delay in ms

    while (true) {
        // Check if the aimbot key is pressed
        bool aimbotActive = (GetAsyncKeyState(cfg::aimkey) & 0x8000);  // Assuming cfg::aimbot_key is your aimbot key
        bool triggerbotActive = (GetAsyncKeyState(cfg::triggerbot_key) & 0x8000);  // Assuming cfg::triggerbot_key is your triggerbot key

        if (aimbotActive || triggerbotActive) {  // Only scan when either aimbot or triggerbot key is pressed
            int w, h;
            {
                std::lock_guard<std::mutex> lock(fovMutex);
                w = currentFOV;
                h = currentFOV;
            }

            // Allocate resources if resolution changed
            if (w != lastW || h != lastH) {
                if (hBitmap) {
                    DeleteObject(hBitmap);
                    hBitmap = nullptr;
                }
                if (screenData) {
                    free(screenData);
                    screenData = nullptr;
                }

                hBitmap = CreateCompatibleBitmap(hScreen, w, h);
                screenData = (BYTE*)malloc(w * h * 4);  // 4 bytes per pixel (RGBA)

                lastW = w;
                lastH = h;

                if (!hBitmap || !screenData) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cfg::sleep));
                    continue;
                }
            }

            if (!skipNextFrames) {
                // Capture region around center
                SelectObject(hDC, hBitmap);
                BitBlt(hDC, 0, 0, w, h, hScreen, middle_screen.x - w / 2, middle_screen.y - h / 2, SRCCOPY);

                // Prepare BITMAPINFO structure
                BITMAPINFO bmi = {};
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = w;
                bmi.bmiHeader.biHeight = -h; // Top-down DIB
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                // Copy bitmap to buffer
                if (GetDIBits(hDC, hBitmap, 0, h, screenData, &bmi, DIB_RGB_COLORS)) {
                    ProcessImage(screenData, w, h);  // Your color/target logic
                    if (triggerbotActive) {
                        Triggerbot();  // Trigger if lock is valid
                    }
                }
            }

            if (moved_mouse && aimbotActive) {
                Aimbot(aim_x, aim_y, cfg::aimbot_smooth);
                Magnet(aim_x, aim_y, cfg::aimassist_smooth);
                moved_mouse = false;
            }

            // Introduce random sleep time to avoid fixed timing patterns.
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % (randomDelayMax - randomDelayMin + 1) + randomDelayMin));

            skipNextFrames = false;
            moved_mouse = true;
        }
        else {
            // Sleep if neither aimbot nor triggerbot key is pressed to save resources
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg::sleep));
        }
    }

    // Cleanup – won't reach here, but good practice
    if (hBitmap) DeleteObject(hBitmap);
    if (hDC) DeleteDC(hDC);
    if (hScreen) ReleaseDC(NULL, hScreen);
    if (screenData) free(screenData);
}
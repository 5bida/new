
#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <windows.h>
#include <chrono>
#include "Stopwatch.hpp"
#include <memory>
#include <mutex>
#include "Driver.hpp"

Driver::Comms XD;
std::mutex fovMutex;
int currentFOV = 0;
bool key_ativa = false;

#define M_PI 3.141592653589793238462643383279502884197169

int aim_x = 0;
int aim_y = 0;
int Width;
int Height;
int oX, oY;
int silent_x, silent_y;
int fov = 0;

struct Point {
    double x, y;
    Point(double _x = 0, double _y = 0) : x(_x), y(_y) {}
};

namespace AimbotRNG {
    static std::random_device& GetRandomDevice() {
        static std::random_device rd;
        return rd;
    }

    static std::mt19937& GetGenerator() {
        static std::mt19937 gen(GetRandomDevice()());
        return gen;
    }

    static int GetRandomDelay(int min, int max) {
        int adjustedMax = (max - min > 3) ? min + 3 : max;
        std::uniform_int_distribution<> dis(min, adjustedMax);
        return dis(GetGenerator());
    }

    static double GetHumanizedMultiplier() {
        std::normal_distribution<> dis(1.0, 0.15);
        double multiplier = dis(GetGenerator());
        return std::clamp(multiplier, 0.85, 1.15);
    }

    static double AddHumanNoise(double value, double intensity = 0.08) {
        std::normal_distribution<> dis(0, intensity);
        return value * (1.0 + dis(GetGenerator()) * 0.3);
    }

    static double AddMicroAdjustment(double value, double intensity = 0.04) {
        std::normal_distribution<> dis(0, intensity);
        return value * (1.0 + dis(GetGenerator()) * 0.2);
    }
}

void add_overflow(double Input, double& Overflow) {
    Overflow = std::modf(Input, &Input) + Overflow;
    if (Overflow > 1.0) {
        double Integral{ 0.0 };
        Overflow = std::modf(Overflow, &Integral);
        Input += Integral;
    }
}

static bool InsideCircleTrigger(float centerX, float centerY, float fovX, float fovY, float x, float y) {
    float minX = centerX - fovX / 2;
    float maxX = centerX + fovX / 2;
    float minY = centerY - fovY / 2;
    float maxY = centerY + fovY / 2;
    return x >= minX && x <= maxX && y >= minY && y <= maxY;
}

float DistanceBetweenCross(float X, float Y) {
    float ydist = (Y - (Height / 2));
    float xdist = (X - (Width / 2));
    float Hypotenuse = sqrt(pow(ydist, 2) + pow(xdist, 2));
    return Hypotenuse;
}

Point getBezierPoint(const std::vector<Point>& points, double t) {
    if (points.empty()) return Point();

    std::vector<Point> temp = points;
    for (int i = points.size() - 1; i > 0; i--) {
        for (int j = 0; j < i; j++) {
            temp[j].x = temp[j].x * (1 - t) + temp[j + 1].x * t;
            temp[j].y = temp[j].y * (1 - t) + temp[j + 1].y * t;
        }
    }
    return temp[0];
}

void Aimbot(int aimX, int aimY, double smooth) {
    if (!cfg::aimbot_ativo || !GetAsyncKeyState(cfg::aimkey)) {
        return;
    }

    std::lock_guard<std::mutex> lock(fovMutex);
    currentFOV = cfg::aimbot_fov;

    static Point lastMove(0, 0);
    static int consecutiveMoves = 0;
    double x_{ 0.0 }, y_{ 0.0 }, overflow_x{ 0.0 }, overflow_y{ 0.0 };

    std::vector<Point> controlPoints = {
        Point(0, 0),
        Point(aimX * 0.75 * AimbotRNG::GetHumanizedMultiplier(),
              aimY * 0.75 * AimbotRNG::GetHumanizedMultiplier()),
        Point(aimX * 0.85 * AimbotRNG::GetHumanizedMultiplier(),
              aimY * 0.85 * AimbotRNG::GetHumanizedMultiplier()),
        Point(aimX, aimY)
    };

    double actualSmooth = (smooth * 0.45 < 1.6) ? 1.6 : smooth * 0.45;
    actualSmooth *= AimbotRNG::GetHumanizedMultiplier();

    double speedMultiplier = (cfg::speed * 1.7 > 2.3) ? 2.3 : cfg::speed * 1.7;
    speedMultiplier *= AimbotRNG::GetHumanizedMultiplier();

    double distance = sqrt(aimX * aimX + aimY * aimY);
    double speedFactor = (distance / 85.0 > 1.15) ? 1.15 : distance / 85.0;
    speedFactor = AimbotRNG::AddMicroAdjustment(speedFactor + 0.65);

    if (distance > 100 && consecutiveMoves > 15) {
        std::this_thread::sleep_for(std::chrono::microseconds(
            AimbotRNG::GetRandomDelay(100, 200)));
        consecutiveMoves = 0;
    }

    for (int i = 1; i <= actualSmooth; i++) {
        double t = pow(i / actualSmooth, 0.55 + AimbotRNG::AddMicroAdjustment(0, 0.02));
        Point curvePoint = getBezierPoint(controlPoints, t);

        int newX = static_cast<int>(curvePoint.x * speedFactor);
        int newY = static_cast<int>(curvePoint.y * speedFactor);

        double interpFactor = 0.93 + AimbotRNG::AddMicroAdjustment(0, 0.01);
        newX = static_cast<int>(newX * interpFactor + lastMove.x * (1 - interpFactor));
        newY = static_cast<int>(newY * interpFactor + lastMove.y * (1 - interpFactor));

        add_overflow(newX, overflow_x);
        add_overflow(newY, overflow_y);

        int final_x = static_cast<int>(newX - x_);
        int final_y = static_cast<int>(newY - y_);

        final_x = static_cast<int>(final_x * speedMultiplier *
            AimbotRNG::GetHumanizedMultiplier());
        final_y = static_cast<int>(final_y * speedMultiplier *
            AimbotRNG::GetHumanizedMultiplier());

        if (abs(final_x) < 0.6 && abs(final_y) < 0.6) {
            final_x = 0;
            final_y = 0;
        }

        int baseDelay = (consecutiveMoves < 5) ?
            AimbotRNG::GetRandomDelay(12, 25) :
            AimbotRNG::GetRandomDelay(8, 20);

        std::this_thread::sleep_for(std::chrono::microseconds(baseDelay));

        if (cfg::recoil_ativo) {
            double recoilOffset = cfg::recoil_offset *
                AimbotRNG::GetHumanizedMultiplier();
            XD.MouseEvent(final_x, final_y + recoilOffset, Driver::None);
        }
        else {
            XD.MouseEvent(final_x, final_y, Driver::None);
        }

        x_ = newX;
        y_ = newY;
        lastMove = Point(final_x, final_y);
        consecutiveMoves++;
    }

    if (consecutiveMoves > 20) {
        consecutiveMoves = 0;
    }
}

void recoil_control() {
    stopwatch timer;
    static double lastRecoil = 0;

    while (true) {
        if ((GetAsyncKeyState)(cfg::recoil_key) && cfg::recoil_ativo) {
            if (timer.get_elapsed() > cfg::time_to_start) {
                if (cfg::recoil_offset < cfg::recoil_length) {
                    double targetRecoil = cfg::recoil_speed * 1.4 *
                        (1.0 + AimbotRNG::AddHumanNoise(0.06));
                    cfg::recoil_offset += targetRecoil;
                    lastRecoil = targetRecoil;
                }
                else {
                    cfg::recoil_offset = cfg::recoil_length;
                }
            }
        }
        else {
            if (cfg::recoil_offset > 0) {
                double resetSpeed = cfg::recoil_speed * 1.3;
                cfg::recoil_offset -= resetSpeed;
                lastRecoil = 0;
            }
            else {
                cfg::recoil_offset = 0;
                timer.update();
            }
        }

        if (!(GetAsyncKeyState)(cfg::aimkey)) {
            cfg::recoil_offset = 0;
            lastRecoil = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Magnet(int aimX, int aimY, double smooth) {
    static std::vector<Point> previousPoints;
    static Point lastMove(0, 0);

    if (cfg::aimassist_ativo) {
        std::lock_guard<std::mutex> lock(fovMutex);
        currentFOV = cfg::aimassist_fov;

        std::vector<Point> controlPoints = {
            Point(0, 0),
            Point(aimX * 0.6, aimY * 0.6),
            Point(aimX * 0.8, aimY * 0.8),
            Point(aimX, aimY)
        };

        if (!previousPoints.empty()) {
            for (size_t i = 0; i < controlPoints.size(); i++) {
                controlPoints[i].x = controlPoints[i].x * 0.9 + previousPoints[i].x * 0.1;
                controlPoints[i].y = controlPoints[i].y * 0.9 + previousPoints[i].y * 0.1;
            }
        }

        double x_{ 0.0 }, y_{ 0.0 }, overflow_x{ 0.0 }, overflow_y{ 0.0 };
        double actualSmooth = (smooth * 0.6 < 2.0) ? 2.0 : smooth * 0.6;
        double speedMultiplier = (cfg::assist_speed * 1.5 > 2.0) ? 2.0 : cfg::assist_speed * 1.5;

        double distance = sqrt(aimX * aimX + aimY * aimY);
        double speedFactor = (distance / 100.0 > 1.0) ? 1.0 : distance / 100.0;
        speedFactor += 0.5;

        for (int i = 1; i <= actualSmooth; i++) {
            double t = pow(i / actualSmooth, 0.7);
            Point curvePoint = getBezierPoint(controlPoints, t);

            int newX = static_cast<int>(curvePoint.x * speedFactor);
            int newY = static_cast<int>(curvePoint.y * speedFactor);

            newX = static_cast<int>(newX * 0.9 + lastMove.x * 0.1);
            newY = static_cast<int>(newY * 0.9 + lastMove.y * 0.1);

            add_overflow(newX, overflow_x);
            add_overflow(newY, overflow_y);

            int final_x = static_cast<int>(newX - x_);
            int final_y = static_cast<int>(newY - y_);

            final_x = static_cast<int>(final_x * speedMultiplier);
            final_y = static_cast<int>(final_y * speedMultiplier);

            if (abs(final_x) < 1 && abs(final_y) < 1) {
                final_x = 0;
                final_y = 0;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(
                AimbotRNG::GetRandomDelay(20, 50)));

            XD.MouseEvent(final_x, final_y, Driver::None);

            x_ = newX;
            y_ = newY;
            lastMove = Point(final_x, final_y);
        }

        previousPoints = controlPoints;
    }
}
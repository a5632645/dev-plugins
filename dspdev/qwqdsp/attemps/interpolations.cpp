#include <algorithm>
#include <array>
#include <cstddef>

#include <raylib.h>
#include "qwqdsp/interpolation/makima.hpp"
#include "qwqdsp/interpolation/catmull_rom_spline.hpp"
#include "qwqdsp/interpolation/sppchip.hpp"
#include "qwqdsp/interpolation/linear.hpp"

int main() {
    size_t draging_obj = -1;
    std::array<Vector2, 11> drags{
        Vector2{50, 300},
        {100, 300},
        {150, 300},
        {200, 200},
        {250, 100},
        {300, 100},
        {350, 100},
        {400, 100},
        {410, 100},
        {420, 100},
        {430, 100},
    };

    InitWindow(500, 500, "interpolation");
    SetTargetFPS(30);
    while (!WindowShouldClose()) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if (draging_obj == -1) {
                for (size_t i = 0; i < drags.size(); ++i) {
                    Rectangle rect;
                    rect.x = drags[i].x - 3;
                    rect.y = drags[i].y - 3;
                    rect.width = 6;
                    rect.height = 6;
                    if (CheckCollisionPointRec(GetMousePosition(), rect)) {
                        draging_obj = i;
                        break;
                    }
                }
            }
            else {
                drags[draging_obj] = GetMousePosition();
            }
        }
        else {
            draging_obj = -1;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        for (auto& drag : drags) {
            DrawRectangleLines(drag.x - 3, drag.y - 3, 6, 6, WHITE);
        }

        auto copy = drags;
        std::sort(copy.begin(), copy.end(), [](auto max, auto now) {
            return max.x < now.x;
        });

        std::array<float, copy.size()> xs;
        std::array<float, copy.size()> ys;
        for (size_t i = 0; i < copy.size(); ++i) {
            xs[i] = copy[i].x;
            ys[i] = copy[i].y;
        }

        qwqdsp::interpolation::SPPCHIP pchip;
        pchip.Reset(xs, ys);
        Vector2 line_start = copy[0];
        for (size_t i = copy[0].x; i <= copy.back().x; ++i) {
            auto y = pchip.Next(i);
            DrawLine(line_start.x, line_start.y, i, y, BLUE);
            line_start.x = i;
            line_start.y = y;
        }

        qwqdsp::interpolation::CatmullRomSpline spline;
        spline.Reset(xs, ys);
        line_start = copy[0];
        for (size_t i = copy[0].x; i <= copy.back().x; ++i) {
            auto y = spline.Next(i);
            DrawLine(line_start.x, line_start.y, i, y, RED);
            line_start.x = i;
            line_start.y = y;
        }

        qwqdsp::interpolation::Makima makima;
        makima.Reset(xs, ys);
        line_start = copy[0];
        for (size_t i = copy[0].x; i <= copy.back().x; ++i) {
            auto y = makima.Next(i);
            DrawLine(line_start.x, line_start.y, i, y, ORANGE);
            line_start.x = i;
            line_start.y = y;
        }

        qwqdsp::interpolation::Linear linear;
        linear.Reset(xs, ys);
        line_start = copy[0];
        for (size_t i = copy[0].x; i <= copy.back().x; ++i) {
            auto y = linear.Next(i);
            DrawLine(line_start.x, line_start.y, i, y, GREEN);
            line_start.x = i;
            line_start.y = y;
        }

        EndDrawing();
    }
    CloseWindow();
}
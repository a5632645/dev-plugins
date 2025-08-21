#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>
// #include "OLEDDisplayRGB.h"
// #include "stb_image_write.h"
#include <raylib.h>

#include "qwqdsp/osciilor/noise.hpp"
#include "qwqdsp/interpolation4.hpp"

// struct Canvas {
//     int width;
//     int height;
//     static constexpr int bpp = 4;

//     Canvas(int width, int height)
//         : width{width}
//         , height{height}
//         , pixels_(width * height)
//         , g(width, height)
//     {
//         g.SetDisplayBuffer(pixels_.data());
//     }

//     const auto& GetPixels() const { return pixels_; }

//     void SaveImage(std::string_view path) {
//         stbi_write_png(path.data(), width, height, 4,
//                    pixels_.data(), width * bpp);
//     }

//     OLEDDisplay g;
// private:
//     std::vector<OLEDRGBColor> pixels_;
// };

int main() {
    size_t draging_obj = -1;
    std::array<Vector2, 4> drags{
        Vector2{100, 200},
        {150, 200},
        {200, 200},
        {250, 200}
    };

    InitWindow(400, 400, "playing");
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

        {
            auto line_start = copy[0];
            for (size_t i = copy[0].x; i < copy[1].x; ++i) {
                auto y = qwqdsp::Interpolation4::Lagrange3rd(
                    copy[0].y, copy[1].y, copy[2].y, copy[3].y,
                    copy[0].x, copy[1].x, copy[2].x, copy[3].x,
                    i
                );
                DrawLine(line_start.x, line_start.y, i, y, WHITE);
                line_start.x = i;
                line_start.y = y;
            }
            for (size_t i = copy[1].x; i < copy[2].x; ++i) {
                auto y = qwqdsp::Interpolation4::Lagrange3rd(
                    copy[0].y, copy[1].y, copy[2].y, copy[3].y,
                    copy[0].x, copy[1].x, copy[2].x, copy[3].x,
                    i
                );
                DrawLine(line_start.x, line_start.y, i, y, WHITE);
                line_start.x = i;
                line_start.y = y;
            }
            for (size_t i = copy[2].x; i < copy[3].x; ++i) {
                auto y = qwqdsp::Interpolation4::Lagrange3rd(
                    copy[0].y, copy[1].y, copy[2].y, copy[3].y,
                    copy[0].x, copy[1].x, copy[2].x, copy[3].x,
                    i
                );
                DrawLine(line_start.x, line_start.y, i, y, WHITE);
                line_start.x = i;
                line_start.y = y;
            }
        }

        {
            auto line_start = copy[0];
            for (size_t i = copy[0].x; i < copy[1].x; ++i) {
                auto y = qwqdsp::Interpolation4::CatmullRomSpline(
                    copy[0].y, copy[1].y, copy[2].y, copy[3].y,
                    copy[0].x, copy[1].x, copy[2].x, copy[3].x,
                    i, 0.0f
                );
                DrawLine(line_start.x, line_start.y, i, y, RED);
                line_start.x = i;
                line_start.y = y;
            }
            for (size_t i = copy[1].x; i < copy[2].x; ++i) {
                auto y = qwqdsp::Interpolation4::CatmullRomSpline(
                    copy[0].y, copy[1].y, copy[2].y, copy[3].y,
                    copy[0].x, copy[1].x, copy[2].x, copy[3].x,
                    i, 0.0f
                );
                DrawLine(line_start.x, line_start.y, i, y, RED);
                line_start.x = i;
                line_start.y = y;
            }
            for (size_t i = copy[2].x; i < copy[3].x; ++i) {
                auto y = qwqdsp::Interpolation4::CatmullRomSpline(
                    copy[0].y, copy[1].y, copy[2].y, copy[3].y,
                    copy[0].x, copy[1].x, copy[2].x, copy[3].x,
                    i, 0.0f
                );
                DrawLine(line_start.x, line_start.y, i, y, RED);
                line_start.x = i;
                line_start.y = y;
            }
        }

        EndDrawing();
    }
    CloseWindow();
}
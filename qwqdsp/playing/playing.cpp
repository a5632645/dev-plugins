#include <algorithm>
#include <raylib.h>
#include <numbers>
#include <cmath>
#include <string>

#include "qwqdsp/fastmath/math.hpp"
#include "qwqdsp/fastmath/sin.hpp"

static constexpr float jmap(float x, float xmin, float xmax, float ymin, float ymax) noexcept {
    return (x - xmin) / (xmax - xmin) * (ymax - ymin) + ymin;
}

int main() {
    InitWindow(1280, 720, "remez");
    SetTargetFPS(30);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BLACK);
        float x1 = 0;
        float y1 = 0;
        float x2 = 0;
        float y2 = 0;
        float differences[1280];
        for (int i = 0; i < 1280; ++i) {
            float const x = i / 1280.0f * std::numbers::pi_v<float>;
            float xy1 = qwqdsp::fastmath::Exp2(x);
            float xy2 = std::exp2(x);
            float newy1 = jmap(xy1, -2, 2, 720, 0);
            float newy2 = jmap(xy2, -2, 2, 720, 0);
            DrawLine(x1, y1, i, newy1, WHITE);
            DrawLine(x2, y2, i, newy2, RED);
            x1 = i;
            x2 = x1;
            y1 = newy1;
            y2 = newy2;
            differences[i] = std::abs(xy1 - xy2);
        }
        float const max = *std::max_element(differences, differences + 1280);
        DrawText(std::to_string(max).c_str(), 0, 0, 20, WHITE);
        x1 = 0;
        y1 = 0;
        for (size_t i = 0; i < 1280; ++i) {
            float const v = differences[i] / max;
            float y = jmap(v, 0, 1.5, 720, 0);
            DrawLine(x1, y1, i, y, GREEN);
            x1 = i;
            y1 = y;
        }

        EndDrawing();
    }

    CloseWindow();
}
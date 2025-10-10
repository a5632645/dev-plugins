#include <raylib.h>
#include "raymath.h"

constexpr int kWidth = 640;
constexpr int kHeight = 480;

static Vector3 camera_pos{};
static float viewport_scale = 1.0f / 480.0f;
static float viewport_z = 1;

static Vector3 GetViewportPoint(float u, float v) {
    float viewport_width = kWidth * viewport_scale;
    float viewport_height = kHeight * viewport_scale;
    float x = -viewport_width / 2 + viewport_width * u;
    float y = viewport_height / 2 - viewport_height * v;
    return {x, y, viewport_z};
}

struct color {
    float r;
    float g;
    float b;

    operator Color() const noexcept {
        Color c;
        c.a = 255;
        c.r = r * 255.9;
        c.g = g * 255.9;
        c.b = b * 255.9;
        return c;
    }

    color& operator*=(float v) noexcept {
        r *= v;
        g *= v;
        b *= v;
        return *this;
    }

    color operator+=(color& a) noexcept {
        r += a.r;
        g += a.g;
        b += a.b;
        return *this;
    }
};
static color operator*(color a, float v) noexcept {
    a *= v;
    return a;
}
static color operator*(float v, color a) noexcept {
    a *= v;
    return a;
}
static color operator+(color a, color b) noexcept {
    a += b;
    return a;
}

struct MySphere {
    Vector3 pos{};
    float radius{};
};

static MySphere ball{
    .pos = {0, 0, 0},
    .radius = 5
};

static Color RayColor(Ray ray) {
    ray.direction = Vector3Normalize(ray.direction);

    auto coll = GetRayCollisionSphere(ray, ball.pos, ball.radius);
    if (coll.hit) {
       return color{coll.normal.x+1, coll.normal.y+1, coll.normal.z+1}*0.5f;
    }

    auto a = 0.5*(ray.direction.y + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}

int main() {
    InitWindow(kWidth, kHeight, "raytracing");
    SetTargetFPS(30);

    while (!WindowShouldClose()) {
        BeginDrawing();

        for (int x = 0; x < kWidth; ++x) {
            for (int y = 0; y < kHeight; ++y) {
                Vector3 viewport = GetViewportPoint(x / static_cast<float>(kWidth), y / static_cast<float>(kHeight));
                Ray ray;
                ray.position = camera_pos;
                ray.direction = viewport;

                Color c = RayColor(ray);
                DrawPixel(x, y, c);
            }
        }

        EndDrawing();
    }

    CloseWindow();
}
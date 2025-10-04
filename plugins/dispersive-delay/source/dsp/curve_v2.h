#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <nlohmann/json_fwd.hpp>
#include "listener_list.h"

namespace mana {
class CurveV2 {
public:
    static constexpr auto kLineResolution = 1024;

    enum class CurveInitEnum {
        kRamp,
        kNull,
        kFull
    };

    enum class PowerEnum {
        kKeep = 0,
        kExp,
        kWaveSine,
        kWaveTri,
        kWaveSquare,
        kNumPowerEnums
    };
    static float GetPowerYValue(float nor_x, PowerEnum power_type, float power);

    struct Point {
        constexpr Point(float x, float y, float power = 0.0f, PowerEnum power_type = PowerEnum::kExp)
            : x(x), y(y), power(power), power_type(power_type) {}

        float x;
        float y;
        float power;
        PowerEnum power_type{ PowerEnum::kExp };
    };

    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void OnAddPoint(CurveV2* generator, Point p, int before_idx) = 0;
        virtual void OnRemovePoint(CurveV2* generator, int remove_idx) = 0;
        virtual void OnPointXyChanged(CurveV2* generator, int changed_idx) = 0;
        virtual void OnPointPowerChanged(CurveV2* generator, int changed_idx) = 0;
        virtual void OnReload(CurveV2* generator) = 0;
    };

    CurveV2(int size = kLineResolution, CurveInitEnum init = CurveInitEnum::kNull);
    CurveV2(const CurveV2&) = delete;
    CurveV2& operator=(const CurveV2&) = delete;
    CurveV2(CurveV2&&) = default;
    CurveV2& operator=(CurveV2&&) = default;

    void Init(CurveInitEnum init);

    void Remove(int idx);
    void AddBehind(int idx, Point point);
    void AddPoint(Point point);

    decltype(auto) GetDatas() const { return (datas_); }
    float Get(int idx) { return datas_[idx]; }
    float Get(float idx) {
        auto before = static_cast<int>(idx);
        auto after = before + 1;
        auto frac = idx - before;
        return std::lerp(Get(before), Get(after), frac);
    }
    float GetNormalize(float nor) { return Get(num_data_ * nor); }

    decltype(auto) GetAllPoints() { return (points_); }
    decltype(auto) GetAllPoints() const { return (points_); }
    int GetNumPoints() const { return static_cast<int>(points_.size()); }
    Point GetPoint(int idx) { return points_[idx]; }

    void SetXy(int idx, float new_x, float new_y);
    void SetPower(int idx, float new_power);
    void SetPowerType(int idx, PowerEnum new_type);

    void PartRender(int begin_point_idx, int end_point_idx);
    void FullRender() { PartRender(0, static_cast<int>(points_.size())); }

    void AddListener(Listener* l) { listeners_.AddListener(l); }
    void RemoveListener(Listener* l) { listeners_.RemoveListener(l); }

    nlohmann::json SaveState() const;
    void LoadState(const nlohmann::json& j);
private:
    int num_data_;
    std::vector<Point> points_;
    std::vector<float> datas_;
    utli::ListenerList<Listener> listeners_;
};
}
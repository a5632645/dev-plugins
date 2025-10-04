#include "curve_v2.h"

#include <cassert>
#include <cmath>
#include <numbers>
#include <nlohmann/json.hpp>

namespace mana {
float CurveV2::GetPowerYValue(float nor_x, PowerEnum power_type, float power) {
    switch (power_type) {
    case PowerEnum::kKeep:
        return 0.0f;
    case PowerEnum::kExp:
    {
        constexpr auto max_pow = 20;
        auto mapped_exp_base = power * max_pow;
        if (std::abs(mapped_exp_base) <= 1e-3) // almost line
            return nor_x;

        auto down = std::exp(mapped_exp_base) - 1.0f;
        auto up = std::exp(mapped_exp_base * nor_x) - 1.0f;
        return up / down;
    }
    case PowerEnum::kWaveSine:
    {
        constexpr auto max_cycles = 64.0f;
        auto map_v = power * 0.5f + 0.5f;
        auto cycles = std::round(map_v * max_cycles) + 0.5f;
        auto cos_v = -std::cos(cycles * nor_x * std::numbers::pi_v<float> *2.0f);
        return cos_v * 0.5f + 0.5f;
    }
    case PowerEnum::kWaveTri:
    {
        constexpr auto max_cycles = 64.0f;
        auto map_v = power * 0.5f + 0.5f;
        auto cycles = std::round(map_v * max_cycles) + 0.5f;
        float tmp{};
        auto phase = std::modf(nor_x * cycles, &tmp);
        return 1.0f - std::abs(1.0f - 2.0f * phase);
    }
    case PowerEnum::kWaveSquare:
    {
        constexpr auto max_cycles = 63.0f;
        auto map_v = power * 0.5f + 0.5f;
        auto cycles = std::round(map_v * max_cycles) + 1.0f;
        float tmp{};
        auto phase = std::modf(nor_x * cycles, &tmp);
        return phase < 0.5f ? 0.0f : 1.0f;
    }
    default:
        assert(false);
        return 0.0f;
    }
}
}

namespace mana {
CurveV2::CurveV2(int size, CurveInitEnum init) : num_data_(size) {
    datas_.resize(size + 2);
    Init(init);
}

void CurveV2::Init(CurveInitEnum init)
{
    points_.clear();
    switch (init) {
    case CurveInitEnum::kRamp:
        points_.emplace_back(0.0f, 0.0f);
        points_.emplace_back(1.0f, 1.0f);
        break;
    case CurveInitEnum::kNull:
        points_.emplace_back(0.0f, 0.0f);
        points_.emplace_back(1.0f, 0.0f);
        break;
    case CurveInitEnum::kFull:
        points_.emplace_back(0.0f, 1.0f);
        points_.emplace_back(1.0f, 1.0f);
        break;
    default:
        assert(false);
        break;
    }
    FullRender();
    listeners_.CallListener(&Listener::OnReload, this);
}

void CurveV2::Remove(int idx) {
    if (idx == 0 || idx == GetNumPoints() - 1) // do not remove first and last
        return;

    points_.erase(points_.cbegin() + idx);
    PartRender(idx - 1, idx + 1);
    listeners_.CallListener(&Listener::OnRemovePoint, this, idx);
}

void CurveV2::AddPoint(Point point) {
    const size_t num_loop = points_.size() - 1;
    for (size_t i = 0; i < num_loop; ++i) {
        if (point.x >= points_[i].x
            && point.x <= points_[i + 1].x) {
            AddBehind(i, point);
            return;
        }
    }
    AddBehind(points_.size() - 1, point);
}

void CurveV2::AddBehind(int idx, Point point) {
    points_.emplace(points_.begin() + idx + 1, point);
    PartRender(idx, idx + 2);
    listeners_.CallListener(&Listener::OnAddPoint, this, point, idx);
}

void CurveV2::PartRender(int begin_point_idx, int end_point_idx) {
    begin_point_idx = std::max(0, begin_point_idx);
    end_point_idx = std::min(end_point_idx, static_cast<int>(points_.size()));
    // i think add function will keep order
    // so we can use these to render
    // todo: thread safty
    for (int i = begin_point_idx; i < end_point_idx; ++i) {
        if (i + 1 >= static_cast<int>(points_.size()))
            break;

        const auto& curr_point = points_[i];
        const auto& next_point = points_[i + 1];
        auto begin_idx = static_cast<int>(std::round(curr_point.x * num_data_));
        auto end_idx = static_cast<int>(std::round(next_point.x * num_data_));

        if (begin_idx == end_idx) // do not render because it will divide by 0
            continue;

        auto curr_y = curr_point.y;
        auto next_y = next_point.y;
        auto x_range = end_idx - begin_idx;
        auto fx_range = static_cast<float>(x_range);
        auto inv_range = 1.0f / x_range;
        for (int x = 0; x < x_range; ++x) {
            auto nor_x = x * inv_range;
            auto map_x = CurveV2::GetPowerYValue(nor_x, curr_point.power_type, curr_point.power);
            datas_[x + begin_idx] = std::lerp(curr_y, next_y, map_x);
        }
    }
    datas_[num_data_] = datas_[num_data_ - 1];
    datas_[num_data_ + 1] = datas_[num_data_];
}

void CurveV2::SetXy(int idx, float new_x, float new_y) {
    if (idx >= GetNumPoints())
        return;

    if (idx == 0)   // first
        new_x = 0.0f;
    else if (idx == GetNumPoints() - 1)
        new_x = 1.0f;
    else
        new_x = std::clamp(new_x, points_[idx - 1].x, points_[idx + 1].x);
    new_y = std::clamp(new_y, 0.0f, 1.0f);

    auto old_point = points_[idx];
    if (old_point.x != new_x || old_point.y != new_y) {
        old_point.x = new_x;
        old_point.y = new_y;
        points_[idx] = old_point;
        PartRender(idx - 1, idx + 1);
        listeners_.CallListener(&Listener::OnPointXyChanged, this, idx);
    }
}

void CurveV2::SetPower(int idx, float new_power) {
    if (idx >= GetNumPoints() - 1)
        return;

    new_power = std::clamp(new_power, -1.0f, 1.0f);

    auto old_power = points_[idx].power;
    if (old_power != new_power) {
        points_[idx].power = new_power;
        PartRender(idx, idx + 1);
        listeners_.CallListener(&Listener::OnPointPowerChanged, this, idx);
    }
}

void CurveV2::SetPowerType(int idx, PowerEnum new_type) {
    if (idx >= GetNumPoints() - 1)
        return;

    auto old_power = points_[idx].power_type;
    if (old_power != new_type) {
        points_[idx].power_type = new_type;
        PartRender(idx, idx + 1);
        listeners_.CallListener(&Listener::OnPointPowerChanged, this, idx);
    }
}

static constexpr std::array kPowerTypeNames{
    "keep",
    "exp",
    "wave_sine",
    "wave_tri",
    "wave_square"
};
static_assert(kPowerTypeNames.size() == static_cast<size_t>(CurveV2::PowerEnum::kNumPowerEnums));
inline static constexpr std::string GetPowerTypeName(CurveV2::PowerEnum type) {
    return kPowerTypeNames[static_cast<size_t>(type)];
}
nlohmann::json CurveV2::SaveState() const {
    nlohmann::json out;
    for (const auto& p : points_) {
        out.push_back({
            {"x", p.x},
            {"y",p.y},
            {"power",p.power},
            {"type",GetPowerTypeName(p.power_type)} });
    }
    return out;
}

void CurveV2::LoadState(const nlohmann::json& json) {
    static const auto kNameToEnumMap = []() {
        std::unordered_map<std::string, PowerEnum> out;
        out[kPowerTypeNames[0]] = PowerEnum::kKeep;
        out[kPowerTypeNames[1]] = PowerEnum::kExp;
        out[kPowerTypeNames[2]] = PowerEnum::kWaveSine;
        out[kPowerTypeNames[3]] = PowerEnum::kWaveTri;
        out[kPowerTypeNames[4]] = PowerEnum::kWaveSquare;
        return out;
    }();

    decltype(points_) new_points;
    for (const auto& p : json) {
        auto x = p["x"].get<float>();
        auto y = p["y"].get<float>();
        auto power = p["power"].get<float>();
        auto type = kNameToEnumMap.at(p["type"].get<std::string>());
        new_points.push_back(Point{ x, y, power, type });
    }
    std::ranges::sort(new_points, std::less{}, &Point::x);
    points_ = std::move(new_points);
    FullRender();
    listeners_.CallListener(&Listener::OnReload, this);
}
}
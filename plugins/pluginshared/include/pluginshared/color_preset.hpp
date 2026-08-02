#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace ui {

/**
 * @brief 一套完整的颜色预设（内置定义，后续可扩展）
 *
 * 配置文件保存 "颜色名" : "#RRGGBBAA" 的覆盖项；
 * 启动时先用 default 填充色板，再从配置文件读取覆盖；
 * 点击菜单里的预设时，修改 inline 色板并整体刷写到配置文件。
 */
struct ColorPreset {
    juce::String name;

    // live9 风格核心色
    juce::Colour green_bg;       // #99a196 主背景
    juce::Colour light_green_bg; // #c1cac2 下拉框/滑块背景
    juce::Colour black_bg;       // #161820 图表黑色背景/文字
    juce::Colour line_fore;      // #d64800 图表线
    juce::Colour dial_fore;      // #cc5100 旋钮
    juce::Colour active_bg;      // #c6cd00 开关开/选中
    juce::Colour inactive_bg;    // #778592 开关关/未选中
    juce::Colour orange_fore;    // #f5b126 滑块前景

    // 语义色（散落硬编码归一化）
    juce::Colour white_fore;     // 深色背景上的文字/图形
    juce::Colour curve_bg;       // 曲线编辑器背景
    juce::Colour curve_fore;     // 曲线主描边
    juce::Colour curve_fore_alt; // 曲线内描边
    juce::Colour grid_fore;      // 网格/刻度线
    juce::Colour marker_fore;    // 范围标记线
    juce::Colour warning_fore;   // 警告色
};

namespace detail {
inline const ColorPreset& MakeDefaultPreset() {
    static const ColorPreset preset = [] {
        ColorPreset p;
        p.name = "default";
        p.green_bg = {153, 161, 150};
        p.light_green_bg = {193, 202, 194};
        p.black_bg = {22, 27, 32};
        p.line_fore = {214, 72, 0};
        p.dial_fore = {204, 81, 0};
        p.active_bg = {198, 205, 0};
        p.inactive_bg = {119, 133, 146};
        p.orange_fore = {0xf5, 0xb1, 0x26};
        p.white_fore = juce::Colours::white;
        p.curve_bg = juce::Colours::darkgrey;
        p.curve_fore = juce::Colours::darkgreen;
        p.curve_fore_alt = juce::Colours::lightgreen;
        p.grid_fore = juce::Colours::grey;
        p.marker_fore = juce::Colours::lightblue;
        p.warning_fore = juce::Colours::red;
        return p;
    }();
    return preset;
}

inline const ColorPreset& MakeLive10Preset() {
    static const ColorPreset preset = [] {
        ColorPreset p;
        p.name = "Live10";
        p.green_bg = {168, 168, 168};
        p.light_green_bg = {229, 229, 229};
        p.black_bg = {13, 13, 13};
        p.line_fore = {3, 181, 194};
        p.dial_fore = {0, 238, 255};
        p.active_bg = {255, 185, 1};
        p.inactive_bg = {214, 214, 214};
        p.orange_fore = {0, 202, 219};
        p.white_fore = {255, 255, 255};
        p.curve_bg = {13, 13, 13};
        p.curve_fore = {3, 181, 194};
        p.curve_fore_alt = {3, 181, 194};
        p.grid_fore = {52, 52, 52};
        p.marker_fore = {52, 52, 52};
        p.warning_fore = {255, 0, 0};
        return p;
    }();
    return preset;
}

inline const ColorPreset& MakeLive9Preset() {
    static const ColorPreset preset = [] {
        ColorPreset p;
        p.name = "Live9";
        p.green_bg = {141, 141, 141};
        p.light_green_bg = {191, 192, 192};
        p.black_bg = {24, 30, 34};
        p.line_fore = {247, 107, 2};
        p.dial_fore = {225, 100, 17};
        p.active_bg = {255, 207, 7};
        p.inactive_bg = {153, 153, 153};
        p.orange_fore = {224, 125, 57};
        p.white_fore = {255, 255, 255};
        p.curve_bg = {13, 13, 13};
        p.curve_fore = {3, 181, 194};
        p.curve_fore_alt = {3, 181, 194};
        p.grid_fore = {52, 52, 52};
        p.marker_fore = {52, 52, 52};
        p.warning_fore = {255, 0, 0};
        return p;
    }();
    return preset;
}
} // namespace detail

// ---------------------------------------------------------------------------
// 运行时色板
// ---------------------------------------------------------------------------
inline juce::Colour green_bg = detail::MakeDefaultPreset().green_bg;
inline juce::Colour light_green_bg = detail::MakeDefaultPreset().light_green_bg;
inline juce::Colour black_bg = detail::MakeDefaultPreset().black_bg;
inline juce::Colour line_fore = detail::MakeDefaultPreset().line_fore;
inline juce::Colour dial_fore = detail::MakeDefaultPreset().dial_fore;
inline juce::Colour active_bg = detail::MakeDefaultPreset().active_bg;
inline juce::Colour inactive_bg = detail::MakeDefaultPreset().inactive_bg;
inline juce::Colour orange_fore = detail::MakeDefaultPreset().orange_fore;
inline juce::Colour white_fore = detail::MakeDefaultPreset().white_fore;
inline juce::Colour curve_bg = detail::MakeDefaultPreset().curve_bg;
inline juce::Colour curve_fore = detail::MakeDefaultPreset().curve_fore;
inline juce::Colour curve_fore_alt = detail::MakeDefaultPreset().curve_fore_alt;
inline juce::Colour grid_fore = detail::MakeDefaultPreset().grid_fore;
inline juce::Colour marker_fore = detail::MakeDefaultPreset().marker_fore;
inline juce::Colour warning_fore = detail::MakeDefaultPreset().warning_fore;

/**
 * @brief 颜色预设管理：配置文件只保存 "颜色名":"#RRGGBB" 覆盖项
 *
 * - 启动：LoadColorsFromConfig() 先用 default 填充，再逐项读配置覆盖；
 * - 菜单点击：ApplyAndSavePreset() 修改 inline 色板，并整体刷写配置；
 * - 配置文件不存在/某键缺失/值非法 → 该键保持 default；
 * - 菜单点击预设时不做勾选标记；
 *
 */
class ColorPresetManager {
public:
    inline static const juce::String kDefaultPresetName = "default";

    static juce::StringArray GetPresetNames() {
        juce::StringArray names;
        for (const auto& p : GetBuiltInPresets()) {
            names.add(p.name);
        }
        return names;
    }

    /** 按名字取预设；不存在则回退 default */
    static const ColorPreset& GetPreset(const juce::String& name) {
        for (const auto& p : GetBuiltInPresets()) {
            if (p.name == name) {
                return p;
            }
        }
        return GetBuiltInPresets().front();
    }

    /** 应用预设：整体覆盖当前 DLL 的模块级色板 */
    static void ApplyPreset(const ColorPreset& preset) {
        green_bg = preset.green_bg;
        light_green_bg = preset.light_green_bg;
        black_bg = preset.black_bg;
        line_fore = preset.line_fore;
        dial_fore = preset.dial_fore;
        active_bg = preset.active_bg;
        inactive_bg = preset.inactive_bg;
        orange_fore = preset.orange_fore;
        white_fore = preset.white_fore;
        curve_bg = preset.curve_bg;
        curve_fore = preset.curve_fore;
        curve_fore_alt = preset.curve_fore_alt;
        grid_fore = preset.grid_fore;
        marker_fore = preset.marker_fore;
        warning_fore = preset.warning_fore;
    }

    /** 菜单点击预设：修改 inline 色板，并把整套颜色刷写到配置文件 */
    static void ApplyAndSavePreset(const ColorPreset& preset) {
        ApplyPreset(preset);
        SavePresetToConfig(preset);
    }

    /** 启动/创建编辑器：先用 default 填充色板，再从配置文件读取覆盖 */
    static void LoadColorsFromConfig() {
        ApplyPreset(GetPreset(kDefaultPresetName));

        auto file = MakeConfigFile();
        if (file == nullptr) {
            return;
        }
        ApplyOverride(file.get(), "green_bg", green_bg);
        ApplyOverride(file.get(), "light_green_bg", light_green_bg);
        ApplyOverride(file.get(), "black_bg", black_bg);
        ApplyOverride(file.get(), "line_fore", line_fore);
        ApplyOverride(file.get(), "dial_fore", dial_fore);
        ApplyOverride(file.get(), "active_bg", active_bg);
        ApplyOverride(file.get(), "inactive_bg", inactive_bg);
        ApplyOverride(file.get(), "orange_fore", orange_fore);
        ApplyOverride(file.get(), "white_fore", white_fore);
        ApplyOverride(file.get(), "curve_bg", curve_bg);
        ApplyOverride(file.get(), "curve_fore", curve_fore);
        ApplyOverride(file.get(), "curve_fore_alt", curve_fore_alt);
        ApplyOverride(file.get(), "grid_fore", grid_fore);
        ApplyOverride(file.get(), "marker_fore", marker_fore);
        ApplyOverride(file.get(), "warning_fore", warning_fore);
    }

    /** 在系统文件管理器中打开颜色配置文件所在文件夹 */
    static void RevealConfigFolder() {
        auto file = MakeConfigFile();
        if (file == nullptr) {
            return;
        }
        file->getFile().getParentDirectory().startAsProcess();
    }
private:
    static const std::vector<ColorPreset>& GetBuiltInPresets() {
        static const std::vector<ColorPreset> presets = {
            detail::MakeDefaultPreset(),
            detail::MakeLive10Preset(),
            detail::MakeLive9Preset(),
        };
        return presets;
    }

    /** "#RRGGBB"（大写） */
    static juce::String ToHex(const juce::Colour& c) {
        return c.toString();
    }

    /** 接受 "#RRGGBB" / "RRGGBB" / "#AARRGGBB" / "AARRGGBB" */
    static bool IsValidHex(const juce::String& raw) {
        auto t = raw.trim();
        if (t.startsWithChar('#')) {
            t = t.substring(1);
        }
        return (t.length() == 6 || t.length() == 8) && t.containsOnly("0123456789abcdefABCDEF");
    }

    static void ApplyOverride(juce::PropertiesFile* file, const char* name, juce::Colour& slot) {
        const auto hex = file->getValue(name);
        if (IsValidHex(hex)) {
            slot = juce::Colour::fromString(hex);
        }
    }

    static void SavePresetToConfig(const ColorPreset& preset) {
        auto file = MakeConfigFile();
        if (file == nullptr) {
            return;
        }
        auto& props = file->getAllProperties();
        props.set("green_bg", ToHex(preset.green_bg));
        props.set("light_green_bg", ToHex(preset.light_green_bg));
        props.set("black_bg", ToHex(preset.black_bg));
        props.set("line_fore", ToHex(preset.line_fore));
        props.set("dial_fore", ToHex(preset.dial_fore));
        props.set("active_bg", ToHex(preset.active_bg));
        props.set("inactive_bg", ToHex(preset.inactive_bg));
        props.set("orange_fore", ToHex(preset.orange_fore));
        props.set("white_fore", ToHex(preset.white_fore));
        props.set("curve_bg", ToHex(preset.curve_bg));
        props.set("curve_fore", ToHex(preset.curve_fore));
        props.set("curve_fore_alt", ToHex(preset.curve_fore_alt));
        props.set("grid_fore", ToHex(preset.grid_fore));
        props.set("marker_fore", ToHex(preset.marker_fore));
        props.set("warning_fore", ToHex(preset.warning_fore));
        props.remove("activePreset"); // 清理旧版遗留键
        // getAllProperties() 绕过了 propertyChanged()，needsWriting 不会自动置位，
        // 必须手动标记，否则 saveIfNeeded() 不会真正落盘。
        file->setNeedsToBeSaved(true);
        file->saveIfNeeded();
    }

    /** 厂商级共享配置：所有插件读到同一份文件；临时创建，用完即毁 */
    static std::unique_ptr<juce::PropertiesFile> MakeConfigFile() {
        juce::PropertiesFile::Options options{};
        options.applicationName = JucePlugin_Manufacturer;
        options.filenameSuffix = ".settings";
        options.millisecondsBeforeSaving = 0; // 立即写盘，且不启动 PropertiesFile 内部的 Timer
#if defined(JUCE_LINUX) || defined(JUCE_BSD)
        options.folderName = "~/.config/" JucePlugin_Manufacturer;
#elif defined(JUCE_MAC) || defined(JUCE_IOS)
        options.folderName = JucePlugin_Manufacturer;
#endif
        options.osxLibrarySubFolder = "Application Support";
        options.storageFormat = juce::PropertiesFile::storeAsXML;
        return std::make_unique<juce::PropertiesFile>(options);
    }
};

} // namespace ui

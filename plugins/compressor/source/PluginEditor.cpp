#include "PluginEditor.h"
#include "PluginProcessor.h"

LimiterReduceMeter::LimiterReduceMeter(EmptyAudioProcessor& p)
    : p_(p) {
    startTimerHz(15);
}

void LimiterReduceMeter::paint(juce::Graphics& g) {
    // float reduce_gain = p_.limiter_.GetReduceGain();
    // float reduce_db = qwqdsp::convert::Gain2Db<-60.0f>(reduce_gain);
    // ------------- 1. 设置颜色和基本参数 -------------
    constexpr float meterMinDb = -60.0f; // 仪表的最小显示分贝
    constexpr float meterMaxDb = 0.0f;   // 仪表的最高显示分贝 (0dB 意味着无衰减)
    constexpr float orangeThresholdDb = -1.0f; // 从这个值开始，衰减用橙色显示

    // 获取当前增益衰减值 (线性增益，范围 0.0f 到 1.0f)
    float reduce_gain = p_.limiter_.GetReduceGain();
    
    // 将线性增益转换为分贝 (你的 qwqdsp::convert::Gain2Db 应该处理负无穷)
    float reduce_db = qwqdsp::convert::Gain2Db<meterMinDb>(reduce_gain);

    // 确保 reduce_db 在显示范围内
    reduce_db = juce::jlimit(meterMinDb, meterMaxDb, reduce_db);

    // ------------- 2. 绘制背景 -------------
    g.fillAll(juce::Colours::black); // 黑色背景

    // ------------- 3. 绘制边框 -------------
    g.setColour(juce::Colours::white); // 白色边框
    g.drawRect(getLocalBounds().toFloat(), 1.0f); // 1.0f 是边框宽度

    // ------------- 4. 计算仪表区域和填充高度 -------------
    juce::Rectangle<float> bounds = getLocalBounds().toFloat().reduced(1.0f); // 减去边框的宽度
    float meterHeight = bounds.getHeight();
    
    // 将 reduce_db 映射到 0 到 1 的标准化范围，其中 0dB 对应 0.0，-60dB 对应 1.0
    // (注意：这里的映射是反向的，因为增益衰减是负值，0dB 在顶部)
    float normalizedDb = juce::jmap(reduce_db, meterMinDb, meterMaxDb, 1.0f, 0.0f);
    
    // 计算填充高度 (从顶部开始)
    float fillHeight = normalizedDb * meterHeight;
    
    // 确保填充高度不会超出边界
    fillHeight = juce::jlimit(0.0f, meterHeight, fillHeight);

    // ------------- 5. 绘制增益衰减的方块 -------------
    // 衰减填充区域
    juce::Rectangle<float> fillRect(
        bounds.getX(),
        bounds.getY(), // 从顶部开始
        bounds.getWidth(),
        fillHeight
    );

    // 根据衰减量选择颜色
    // 如果衰减超过某个阈值 (例如 -1dB)，则用橙色
    // 否则用白色 (或灰色)
    if (reduce_db < orangeThresholdDb) {
        g.setColour(juce::Colours::orange);
    } else {
        g.setColour(juce::Colours::grey); // 较小的衰减可以用不那么醒目的颜色
    }
    g.fillRect(fillRect);

    // ------------- 6. 绘制刻度线和文本 -------------
    g.setColour(juce::Colours::white);
    juce::Font font(bounds.getWidth() * 0.4f); // 字体大小按仪表高度的百分比
    g.setFont(font);

    // 刻度线的 X 坐标 (在仪表右侧)
    float tickX = bounds.getRight() - 5.0f; // 距离右边缘 5 像素
    float textX = bounds.getRight() - font.getStringWidth("00") - 2.0f; // 文本的 X 坐标

    // 绘制刻度
    for (float db = meterMaxDb; db >= meterMinDb; db -= 5.0f) // 每 5dB 绘制一个刻度
    {
        // 将分贝值映射到仪表 Y 坐标
        float y = juce::jmap(db, meterMinDb, meterMaxDb, bounds.getBottom(), bounds.getY());

        // 绘制刻度线
        g.drawLine(tickX, y, bounds.getRight(), y, 0.5f); // 0.5f 是线宽

        // 绘制刻度文本 (只在整数 dB 值处显示)
        if (static_cast<int>(db) % 10 == 0) // 每 10dB 显示文本
        {
            juce::String text = juce::String(static_cast<int>(db));
            g.drawText(text, 
                        textX - font.getStringWidth(text) - 2, // 文本的 X 坐标，向左偏移一点
                        std::clamp(y - font.getHeight() / 2, 0.0f, bounds.getHeight() - font.getHeight()), // 文本的 Y 坐标，居中
                        font.getStringWidth(text) + 2, // 文本宽度
                        font.getHeight(), 
                        juce::Justification::centredRight, 
                        true);
        }
    }
}

// ---------------------------------------- editor ----------------------------------------
SteepFlangerAudioProcessorEditor::SteepFlangerAudioProcessorEditor (EmptyAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , meter_(p)
{
    auto& apvts = *p.value_tree_;

    lookahead_.BindParam(apvts, "lookahead");
    addAndMakeVisible(lookahead_);
    release_.BindParam(apvts, "release");
    addAndMakeVisible(release_);
    limit_.BindParam(apvts, "limit");
    addAndMakeVisible(limit_);
    hold_.BindParam(apvts, "hold");
    addAndMakeVisible(hold_);
    makeup_.BindParam(apvts, "makeup");
    addAndMakeVisible(makeup_);

    addAndMakeVisible(meter_);

    setSize(300, 400);
}

SteepFlangerAudioProcessorEditor::~SteepFlangerAudioProcessorEditor() {
}

//==============================================================================
void SteepFlangerAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::green_bg);
}

void SteepFlangerAudioProcessorEditor::resized() {
    auto const bound = getLocalBounds();
    auto b = bound;
    meter_.setBounds(b.removeFromRight(bound.proportionOfWidth(0.2f)).reduced(2,2));
    {
        auto line1 = b.removeFromTop(bound.proportionOfHeight(0.25f));
        lookahead_.setBounds(line1.removeFromLeft(line1.getWidth() / 2));
        limit_.setBounds(line1);
    }
    {
        auto line1 = b.removeFromTop(bound.proportionOfHeight(0.25f));
        hold_.setBounds(line1.removeFromLeft(line1.getWidth() / 2));
        release_.setBounds(line1);
    }
    {
        auto line1 = b.removeFromTop(bound.proportionOfHeight(0.25f));
        makeup_.setBounds(line1.removeFromLeft(line1.getWidth() / 2));
    }
}

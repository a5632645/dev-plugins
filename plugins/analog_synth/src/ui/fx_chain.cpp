#include "fx_chain.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
FxChainGui::FxChainGui(Synth& synth, juce::AudioProcessor& processor)
    : synth_(synth)
    , processor_(processor)
    , chorus_(synth)
    , delay_(synth)
    , distortion_(synth)
    , reverb_(synth)
    , phaser(synth) {
    addAndMakeVisible(chorus_);
    addAndMakeVisible(delay_);
    addAndMakeVisible(distortion_);
    addAndMakeVisible(reverb_);
    addAndMakeVisible(phaser);
    chorus_.addMouseListener(this, false);
    delay_.addMouseListener(this, false);
    distortion_.addMouseListener(this, false);
    reverb_.addMouseListener(this, false);
    phaser.addMouseListener(this, false);
    fx_sections_.push_back(&chorus_);
    fx_sections_.push_back(&delay_);
    fx_sections_.push_back(&distortion_);
    fx_sections_.push_back(&reverb_);
    fx_sections_.push_back(&phaser);
    fx_height_hash_[&chorus_] = 160;
    fx_height_hash_[&delay_] = 160;
    fx_height_hash_[&distortion_] = 90;
    fx_height_hash_[&reverb_] = 160;
    fx_height_hash_[&phaser] = 160;
    fx_ptr_hash_["chorus"] = &chorus_;
    fx_ptr_hash_["delay"] = &delay_;
    fx_ptr_hash_["distortion"] = &distortion_;
    fx_ptr_hash_["reverb"] = &reverb_;
    fx_ptr_hash_["phaser"] = &phaser;
    RebuildInterface();
    startTimerHz(10);
}

void FxChainGui::resized() {
    auto fx_bound = getLocalBounds();
    for (auto* c : fx_sections_) {
        c->setBounds(fx_bound.removeFromTop(fx_height_hash_[c]));
    }
}

void FxChainGui::timerCallback() {
    if (synth_.fx_order_changed.exchange(false)) {
        RebuildInterface();
    }
}

void FxChainGui::RebuildInterface() {
    auto order = synth_.GetCurrentFxChainNames();
    std::vector<juce::Component*> new_order;
    for (auto& name : order) {
        new_order.push_back(fx_ptr_hash_[name]);
    }
    fx_sections_.swap(new_order);
    resized();
}

void FxChainGui::mouseDown(juce::MouseEvent const& e) {
    if (e.originalComponent == this || e.originalComponent == nullptr) return;
    component_dragger_.startDraggingComponent(e.originalComponent, e);
    e.originalComponent->toFront(false);
}

void FxChainGui::mouseUp(juce::MouseEvent const& e) {
    std::ignore = e;
    resized();
}

void FxChainGui::mouseDrag(juce::MouseEvent const& e) {
    // if (e.originalComponent == this || e.originalComponent == nullptr) return;
    // component_dragger_.dragComponent(e.originalComponent, e, nullptr);
    // auto bound = e.originalComponent->getBounds();
    // bound.setX(0);
    // bound.setY(std::clamp(bound.getY(), 0, getHeight() - bound.getHeight()));
    // e.originalComponent->setBounds(bound);

    // int current_y = bound.getCentreY();
    // int y = 0;
    // size_t current_idx = 0;
    // for (current_idx = 0; current_idx < fx_sections_.size(); ++current_idx) {
    //     int curr_height = fx_sections_[current_idx]->getHeight();
    //     if (current_y < y + curr_height) {
    //         break;
    //     }
    //     y += curr_height;
    // }
    // current_idx = std::min(current_idx, fx_sections_.size() - 1);
    // if (e.originalComponent == fx_sections_[current_idx]) return;

    // auto old_index = std::find(fx_sections_.begin(), fx_sections_.end(), e.originalComponent) - fx_sections_.begin();

    // {
    //     juce::ScopedLock lock{processor_.getCallbackLock()};
    //     synth_.MoveFxOrder(old_index, current_idx);
    // }

    // auto it = fx_sections_.begin() + old_index;
    // if (old_index < current_idx) {
    //     auto new_pos = fx_sections_.begin() + current_idx + 1;
    //     std::rotate(it, it + 1, new_pos);
    // }
    // else {
    //     auto start_pos = fx_sections_.begin() + current_idx;
    //     std::rotate(start_pos, it, it + 1);
    // }

    // resized();
    if (e.originalComponent == this || e.originalComponent == nullptr) return;
    
    juce::Component* dragged_comp = e.originalComponent;

    // 1. 拖拽组件边界处理
    component_dragger_.dragComponent(dragged_comp, e, nullptr);
    auto bound = dragged_comp->getBounds();
    bound.setX(0);
    bound.setY(std::clamp(bound.getY(), -bound.getHeight() / 2, getHeight() - bound.getHeight() / 2));
    dragged_comp->setBounds(bound);

    // 2. 获取被拖拽组件的旧索引
    auto old_it = std::find(fx_sections_.begin(), fx_sections_.end(), dragged_comp);
    if (old_it == fx_sections_.end()) return;
    size_t old_index = std::distance(fx_sections_.begin(), old_it);

    // 3. 计算目标索引 (使用中心点和中线判断)
    int dragged_centre_y = bound.getCentreY();
    
    // 目标索引 new_index 
    // 注意：这个索引是它在 fx_sections_ 列表中应该插入的位置
    int new_index = getTargetIndexForUnevenHeights(dragged_centre_y, dragged_comp);
    
    // 修正 new_index：如果向下移动，索引需要减 1，因为列表中少了一个元素
    if (new_index > old_index) {
        new_index--;
    }
    
    // 4. 如果新旧索引相同，返回，不进行任何操作
    if ((size_t)new_index == old_index) {
        return;
    }
    
    // 5. 执行交换和 UI 更新 (与你原始代码相似)
    
    // 锁定音频/数据访问
    {
        juce::ScopedLock lock{processor_.getCallbackLock()};
        // 假设 MoveFxOrder 接受 name 和 index，获取 name
        // juce::StringRef name = dragged_comp->getName(); 
        // synth_.MoveFxOrder(name, new_index); 
        synth_.MoveFxOrder(old_index, new_index);
    }

    // 更新 GUI 容器内部的 Component 列表顺序 (使用 std::rotate)
    auto ui_it = fx_sections_.begin() + old_index;
    if (old_index < (size_t)new_index) {
        // 向下移动
        std::rotate(ui_it, ui_it + 1, fx_sections_.begin() + new_index + 1);
    }
    else {
        // 向上移动
        std::rotate(fx_sections_.begin() + new_index, ui_it, ui_it + 1);
    }

    // 立即重新布局所有组件
    resized();
}

// 应该定义在 FxChainGui 的私有部分或作为成员函数
int FxChainGui::getTargetIndexForUnevenHeights(int dragged_centre_y, juce::Component* dragged_component) const
{
    int current_y_accumulator = 0;
    
    // 遍历当前的组件列表（fx_sections_），这是当前显示的顺序
    for (int i = 0; i < fx_sections_.size(); ++i)
    {
        juce::Component* current_comp = fx_sections_[i];
        
        // 排除被拖拽的组件本身，它在列表中不占用一个槽位
        if (current_comp == dragged_component)
        {
            continue; 
        }

        // 获取当前组件的高度
        // 注意：这里使用 getBounds().getHeight() 或 fx_height_hash_
        // 如果 fx_sections_ 中的组件是 FxItemComponent，直接用 getHeight() 即可。
        int current_comp_height = current_comp->getHeight();
        
        // 计算当前组件的中线 Y 坐标 (在父容器中的绝对坐标)
        int component_midline_y = current_y_accumulator + (current_comp_height / 2);
        
        // 核心判断：如果拖拽组件的中心点 Y 越过了当前组件的中线 Y，
        // 则目标位置应在当前组件之前 (即索引 i)
        if (dragged_centre_y < component_midline_y)
        {
            // 如果是在第 0 个组件之前拖拽，索引就是 0
            return i;
        }

        // 累加高度，准备检查下一个组件
        current_y_accumulator += current_comp_height;
    }

    // 如果循环结束了，说明拖拽组件的中心点在所有组件的中线之下
    // 目标索引应为列表的末尾 (fx_sections_.size())
    return (int)fx_sections_.size();
}
}
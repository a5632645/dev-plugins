#include "couple.hpp"
#include "../PluginProcessor.h"

CoupleGui::CoupleGui(ResonatorAudioProcessor& p) {
    auto& apvts = *p.value_tree_;
    reflection0_.BindParam(apvts, "reflection0");
    addAndMakeVisible(reflection0_);
    reflection1_.BindParam(apvts, "reflection1");
    addAndMakeVisible(reflection1_);
    reflection2_.BindParam(apvts, "reflection2");
    addAndMakeVisible(reflection2_);
    reflection3_.BindParam(apvts, "reflection3");
    addAndMakeVisible(reflection3_);
    reflection4_.BindParam(apvts, "reflection4");
    addAndMakeVisible(reflection4_);
    reflection5_.BindParam(apvts, "reflection5");
    addAndMakeVisible(reflection5_);
    reflection6_.BindParam(apvts, "reflection6");
    addAndMakeVisible(reflection6_);
    reflection7_.BindParam(apvts, "reflection7");
    addAndMakeVisible(reflection7_);
}

void CoupleGui::paint(juce::Graphics& g) {
    g.fillAll(ui::green_bg);
}

void CoupleGui::resized() {
    auto martix_b = getLocalBounds();
    auto w = martix_b.getWidth() / 2;
    {
        auto first4 = martix_b.removeFromLeft(w);
        reflection0_.setBounds(first4.removeFromTop(80));
        reflection1_.setBounds(first4.removeFromTop(80));
        reflection2_.setBounds(first4.removeFromTop(80));
        reflection3_.setBounds(first4.removeFromTop(80));
    }
    {
        auto second2 = martix_b.removeFromLeft(w);
        reflection4_.setBounds(second2.removeFromTop(80));
        reflection5_.setBounds(second2.removeFromTop(80));
        reflection6_.setBounds(second2.removeFromTop(80));
        reflection7_.setBounds(second2.removeFromTop(80));
    }
}

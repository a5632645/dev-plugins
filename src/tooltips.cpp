#include "tooltips.hpp"
#include "juce_core/juce_core.h"
#include "param_ids.hpp"

namespace tooltip {

Tooltips::Tooltips() {
    combbox_ids_[id::combbox::kEnsembleModeNames] = id::combbox::kEnsembleModeNameIds;
    combbox_ids_[id::combbox::kVocoderNames] = id::combbox::kVocoderNameIds;
    combbox_ids_[id::kChannelVocoderMap] = id::combbox::kChannelVocoderMaps;

    MakeEnglishTooltips();
}

void Tooltips::MakeEnglishTooltips() {
    tooltips_.clear();
    labels_.clear();

    labels_[id::kMainGain] = "MODU";
    tooltips_[id::kMainGain] = "control the modulator gain";
    labels_[id::kSideGain] = "CARRY";
    tooltips_[id::kSideGain] = "control the carry gain";
    labels_[id::kOutputgain] = "OUTPUT";
    tooltips_[id::kOutputgain] = "control the output gain";
    labels_[id::kVocoderType] = "TYPE";
    tooltips_[id::kVocoderType] = "change the vocoder type";
    labels_[id::kEmphasisPitch] = "HSHELF";
    tooltips_[id::kEmphasisPitch] = "set modulator high shelf filter cutoff pitch";
    labels_[id::kEmphasisGain] = "GAIN";
    tooltips_[id::kEmphasisGain] = "set modulator high shelf filter gain";
    labels_[id::kEmphasisS] = "SLOPE";
    tooltips_[id::kEmphasisS] = "set modulator high shelf filter slope";
    labels_[id::kShiftPitch] = "PITCH";
    tooltips_[id::kShiftPitch] = "set main pitch shifter, this cause formant shift like effects";

    labels_[id::kForgetRate] = "FORGET";
    tooltips_[id::kForgetRate] = "lpc vocoder's forget rate";
    labels_[id::kLPCSmooth] = "SMOOTH";
    tooltips_[id::kLPCSmooth] = "lpc's lattice coeffient smooth, this cause smear responce";
    labels_[id::kLPCOrder] = "ORDER";
    tooltips_[id::kLPCOrder] = "burg lpc order, higher order take higher cpu";
    labels_[id::kRLSLPCOrder] = "ORDER";
    tooltips_[id::kRLSLPCOrder] = "rls lpc order, higher order take higher cpu";
    labels_[id::kLPCGainAttack] = "ATTACK";
    tooltips_[id::kLPCGainAttack] = "lpc vocoder gain attack";
    labels_[id::kLPCGainRelease] = "RELEASE";
    tooltips_[id::kLPCGainRelease] = "lpc vocoder gain release";
    labels_[id::kLPCDicimate] = "DICIMATE";
    tooltips_[id::kLPCDicimate] = "lpc vocoder dicimate, higher value make a more robotic sound and clearity";

    labels_[id::kStftWindowWidth] = "SMEAR";
    tooltips_[id::kStftWindowWidth] = "expand energy in frequency axis by spectral leak caused by sinc window";
    labels_[id::kStftAttack] = "ATTACK";
    tooltips_[id::kStftAttack] = "bin energy attack in time axis";
    labels_[id::kStftRelease] = "RELEASE";
    tooltips_[id::kStftRelease] = "bin energy release in time axis";
    labels_[id::kStftBlend] = "NOISY";
    tooltips_[id::kStftBlend] = "bin energy remap, higher value cause more noisy sound while lower cause crystal sound";

    labels_[id::kEnsembleDetune] = "DETUNE";
    tooltips_[id::kEnsembleDetune] = "ensemble detune";
    labels_[id::kEnsembleRate] = "RATE";
    tooltips_[id::kEnsembleRate] = "ensemble lfo rate";
    labels_[id::kEnsembleSpread] = "SPREAD";
    tooltips_[id::kEnsembleSpread] = "ensemble spread";
    labels_[id::kEnsembleMix] = "MIX";
    tooltips_[id::kEnsembleMix] = "ensemble mix";
    labels_[id::kEnsembleNumVoices] = "NVOICE";
    tooltips_[id::kEnsembleNumVoices] = "ensemble number of voices";
    labels_[id::kEnsembleMode] = "MODE";
    tooltips_[id::kEnsembleMode] = "ensemble mode";

    labels_[id::kChannelVocoderNBands] = "BANDS";
    tooltips_[id::kChannelVocoderNBands] = "numbers of bands";
    labels_[id::kChannelVocoderFreqBegin] = "FBEGIN";
    tooltips_[id::kChannelVocoderFreqBegin] = "frequency begin";
    labels_[id::kChannelVocoderFreqEnd] = "FEND";
    tooltips_[id::kChannelVocoderFreqEnd] = "frequency end";
    labels_[id::kChannelVocoderAttack] = "ATTACK";
    tooltips_[id::kChannelVocoderAttack] = "envelope follower attack";
    labels_[id::kChannelVocoderRelease] = "RELEASE";
    tooltips_[id::kChannelVocoderRelease] = "envelope follower release";
    labels_[id::kChannelVocoderScale] = "SCALE";
    tooltips_[id::kChannelVocoderScale] = "modulator bandpass bandwidth scale";
    labels_[id::kChannelVocoderCarryScale] = "CSCALE";
    tooltips_[id::kChannelVocoderCarryScale] = "carry bandwidth scale";
    labels_[id::kChannelVocoderMap] = "MAP";
    tooltips_[id::kChannelVocoderMap] = "select bandpass filter distribution type";
    labels_[id::combbox::kChannelVocoderMaps[eChannelVocoderMap_Log]] = "Log";
    labels_[id::combbox::kChannelVocoderMaps[eChannelVocoderMap_Mel]] = "Mel";
    labels_[id::combbox::kChannelVocoderMaps[eChannelVocoderMap_Linear]] = "Linear";

    // guis
    labels_[id::kFilterTitle] = "Pre-Filter";
    labels_[id::kEnsembleTitle] = "Ensemble";
    labels_[id::combbox::kVocoderNameIds[eVocoderType_BurgLPC]] = "Burg-LPC";
    labels_[id::combbox::kVocoderNameIds[eVocoderType_RLSLPC]] = "RLS-LPC";
    labels_[id::combbox::kVocoderNameIds[eVocoderType_STFTVocoder]] = "STFT Vocoder";
    labels_[id::combbox::kVocoderNameIds[eVocoderType_ChannelVocoder]] = "Channel Vocoder";
    labels_[id::combbox::kEnsembleModeNameIds[0]] = "Sine";
    labels_[id::combbox::kEnsembleModeNameIds[1]] = "Noise";
    labels_[id::kRLSTitle] = "RLS-LPC    Warning: Too lound audio will cause unstable";
    labels_[id::kSliderMenuEnterValue] = "Enter Value";
    labels_[id::kMainChannelConfig] = "Modulator Channel";
    labels_[id::kSideChannelConfig] = "Carry Channel";
}

void Tooltips::MakeChineseTooltips() {
    tooltips_.clear();
    labels_.clear();

    labels_[id::kMainGain] = juce::String::fromUTF8("调制源");
    tooltips_[id::kMainGain] = juce::String::fromUTF8("调制源音量");
    labels_[id::kSideGain] = juce::String::fromUTF8("载波");
    tooltips_[id::kSideGain] = juce::String::fromUTF8("载波音量");
    labels_[id::kOutputgain] = juce::String::fromUTF8("输出");
    tooltips_[id::kOutputgain] = juce::String::fromUTF8("输出增益");
    labels_[id::kVocoderType] = juce::String::fromUTF8("类型");
    tooltips_[id::kVocoderType] = juce::String::fromUTF8("更改声码器类型");
    labels_[id::kEmphasisPitch] = juce::String::fromUTF8("高架");
    tooltips_[id::kEmphasisPitch] = juce::String::fromUTF8("调制源高架滤波器截止音高");
    labels_[id::kEmphasisGain] = juce::String::fromUTF8("增益");
    tooltips_[id::kEmphasisGain] = juce::String::fromUTF8("调制源高架滤波器增益");
    labels_[id::kEmphasisS] = juce::String::fromUTF8("斜率");
    tooltips_[id::kEmphasisS] = juce::String::fromUTF8("调制源高架滤波器斜率");
    labels_[id::kShiftPitch] = juce::String::fromUTF8("音高");
    tooltips_[id::kShiftPitch] = juce::String::fromUTF8("调制源音高移动,这会导致共振峰移动的效果");

    labels_[id::kForgetRate] = juce::String::fromUTF8("遗忘");
    tooltips_[id::kForgetRate] = juce::String::fromUTF8("lpc声码器的遗忘速率");
    labels_[id::kLPCSmooth] = juce::String::fromUTF8("平滑");
    tooltips_[id::kLPCSmooth] = juce::String::fromUTF8("burg lpc声码器的格型参数平滑,这会导致听不清的共振峰");
    labels_[id::kLPCOrder] = juce::String::fromUTF8("阶段");
    tooltips_[id::kLPCOrder] = juce::String::fromUTF8("burg lpc声码器的阶段,高阶段需要更多cpu");
    labels_[id::kRLSLPCOrder] = juce::String::fromUTF8("阶段");
    tooltips_[id::kRLSLPCOrder] = juce::String::fromUTF8("rls lpc声码器的阶段,高阶段需要更多cpu");
    labels_[id::kLPCGainAttack] = juce::String::fromUTF8("起音");
    tooltips_[id::kLPCGainAttack] = juce::String::fromUTF8("lpc声码器增益的起音");
    labels_[id::kLPCGainRelease] = juce::String::fromUTF8("释放");
    tooltips_[id::kLPCGainRelease] = juce::String::fromUTF8("lpc声码器增益的释放");
    labels_[id::kLPCDicimate] = juce::String::fromUTF8("抽取");
    tooltips_[id::kLPCDicimate] = juce::String::fromUTF8("lpc声码器的抽取,更高的值制作一种更机械的声音,同时清晰度会增加");

    labels_[id::kStftWindowWidth] = juce::String::fromUTF8("窗宽");
    tooltips_[id::kStftWindowWidth] = juce::String::fromUTF8("通过sinc窗造成的频谱泄露来在频率轴扩展能量");
    labels_[id::kStftAttack] = juce::String::fromUTF8("起音");
    tooltips_[id::kStftAttack] = juce::String::fromUTF8("单音在时间上的能量起音");
    labels_[id::kStftRelease] = juce::String::fromUTF8("释放");
    tooltips_[id::kStftRelease] = juce::String::fromUTF8("单音在时间上的能量释放");
    labels_[id::kStftBlend] = juce::String::fromUTF8("重新映射");
    tooltips_[id::kStftBlend] = juce::String::fromUTF8("重新映射单音的能量,更高的值会导致嘈杂的声音,更低的值会更清晰");

    labels_[id::kEnsembleDetune] = juce::String::fromUTF8("失谐");
    tooltips_[id::kEnsembleDetune] = juce::String::fromUTF8("合唱器的失谐");
    labels_[id::kEnsembleRate] = juce::String::fromUTF8("速率");
    tooltips_[id::kEnsembleRate] = juce::String::fromUTF8("合唱器的调制速率");
    labels_[id::kEnsembleSpread] = juce::String::fromUTF8("扩散");
    tooltips_[id::kEnsembleSpread] = juce::String::fromUTF8("合唱器的声场扩散");
    labels_[id::kEnsembleMix] = juce::String::fromUTF8("混合");
    tooltips_[id::kEnsembleMix] = juce::String::fromUTF8("合唱器的混合");
    labels_[id::kEnsembleNumVoices] = juce::String::fromUTF8("声音");
    tooltips_[id::kEnsembleNumVoices] = juce::String::fromUTF8("合唱器的声音数量");
    labels_[id::kEnsembleMode] = juce::String::fromUTF8("模式");
    tooltips_[id::kEnsembleMode] = juce::String::fromUTF8("合唱器的模式");

    labels_[id::kChannelVocoderNBands] = juce::String::fromUTF8("带数");
    tooltips_[id::kChannelVocoderNBands] = juce::String::fromUTF8("多少个带通滤波器");
    labels_[id::kChannelVocoderFreqBegin] = juce::String::fromUTF8("起始");
    tooltips_[id::kChannelVocoderFreqBegin] = juce::String::fromUTF8("频率起始");
    labels_[id::kChannelVocoderFreqEnd] = juce::String::fromUTF8("结束");
    tooltips_[id::kChannelVocoderFreqEnd] = juce::String::fromUTF8("频率结束");
    labels_[id::kChannelVocoderAttack] = juce::String::fromUTF8("起音");
    tooltips_[id::kChannelVocoderAttack] = juce::String::fromUTF8("包络跟随器起音");
    labels_[id::kChannelVocoderRelease] = juce::String::fromUTF8("释放");
    tooltips_[id::kChannelVocoderRelease] = juce::String::fromUTF8("包络跟随器释放");
    labels_[id::kChannelVocoderScale] = juce::String::fromUTF8("调制源");
    tooltips_[id::kChannelVocoderScale] = juce::String::fromUTF8("调制源带通滤波器带宽缩放");
    labels_[id::kChannelVocoderCarryScale] = juce::String::fromUTF8("载波");
    tooltips_[id::kChannelVocoderCarryScale] = juce::String::fromUTF8("载波带通滤波器带宽缩放");
    labels_[id::kChannelVocoderMap] = juce::String::fromUTF8("分布");
    tooltips_[id::kChannelVocoderMap] = juce::String::fromUTF8("带通滤波器频率分布");
    labels_[id::combbox::kChannelVocoderMaps[eChannelVocoderMap_Log]] = juce::String::fromUTF8("对数");
    labels_[id::combbox::kChannelVocoderMaps[eChannelVocoderMap_Mel]] = juce::String::fromUTF8("梅尔");
    labels_[id::combbox::kChannelVocoderMaps[eChannelVocoderMap_Linear]] = juce::String::fromUTF8("线性");

    // guis
    labels_[id::kFilterTitle] = juce::String::fromUTF8("预滤波");
    labels_[id::kEnsembleTitle] = juce::String::fromUTF8("合唱");
    labels_[id::combbox::kVocoderNameIds[eVocoderType_BurgLPC]] = juce::String::fromUTF8("Burg线性预测");
    labels_[id::combbox::kVocoderNameIds[eVocoderType_RLSLPC]] = juce::String::fromUTF8("RLS线性预测");
    labels_[id::combbox::kVocoderNameIds[eVocoderType_STFTVocoder]] = juce::String::fromUTF8("STFT");
    labels_[id::combbox::kVocoderNameIds[eVocoderType_ChannelVocoder]] = juce::String::fromUTF8("通道声码器");
    labels_[id::combbox::kEnsembleModeNameIds[0]] = juce::String::fromUTF8("正弦");
    labels_[id::combbox::kEnsembleModeNameIds[1]] = juce::String::fromUTF8("噪声");
    labels_[id::kRLSTitle] = juce::String::fromUTF8("RLS线性预测    注意：过大的音频会导致不稳定");
    labels_[id::kSliderMenuEnterValue] = juce::String::fromUTF8("输入值");
    labels_[id::kMainChannelConfig] = juce::String::fromUTF8("调制源通道");
    labels_[id::kSideChannelConfig] = juce::String::fromUTF8("载波通道");
}

}
// #pragma once

// #include <juce_audio_processors/juce_audio_processors.h>
// #include <qwqdsp/adsr_envelope.hpp>
// #include <qwqdsp/convert.hpp>
// #include <qwqdsp/osciilor/vic_sine_osc.hpp>
// #include <qwqdsp/misc/smoother.hpp>
// #include "abstract_synth.hpp"

// class DebugSynth : public AbstractSynth<DebugSynth> {
// public:
//     void StopChannel(int channel) {
//         vol_env_[channel].Noteoff(false);
//     }

//     int FindVoiceToSteal() {
//         int min_channel = 0;
//         float min_vol = std::numeric_limits<float>::infinity();
//         for (auto channel : active_channels_) {
//             if (vol_env_[channel].GetLastOutput() < min_vol) {
//                 min_vol = vol_env_[channel].GetLastOutput();
//                 min_channel = channel;
//             }
//         }
//         return min_channel;
//     }

//     void StartNewChannel(int channel, int note, float velocity, bool retrigger, float glide_begin_pitch, float target_pitch) {
//         if (retrigger) {
//             vol_env_[channel].NoteOn(false);
//         }
//         current_pitch_[channel] = glide_begin_pitch;
//         target_pitch_[channel] = target_pitch;
//     }

//     float GetCurrentGlidingPitch(int channel) {
//         return current_pitch_[channel];
//     }

//     void ProcessAndAddInto(int channel, float* buffer, size_t num_samples) {
//         float temp_buffer[256];

//         vol_env_[channel].Update(qwqdsp::AdsrEnvelope::Parameter{
//             attack, decay, release, fs_, sustain
//         });

//         while (num_samples != 0) {
//             size_t cando = std::min<size_t>(num_samples, 256);
//             vol_env_[channel].ProcessExp({temp_buffer, cando});

//             float pitch_buffer[256];
//             for (size_t i = 0; i < cando; ++i) {
//                 current_pitch_[channel] = target_pitch_[channel] + gliding_factor_ * (current_pitch_[channel] - target_pitch_[channel]);
//                 pitch_buffer[i] = current_pitch_[channel];
//             }

//             oscs_[channel].SetFreq(qwqdsp::convert::Pitch2Freq(pitch_buffer[0]), fs_);
//             for (size_t i = 0; i < cando; ++i) {
//                 buffer[i] += oscs_[channel].Tick() * temp_buffer[i] * 0.3f;
//             }

//             num_samples -= cando;
//             buffer += cando;
//         }
//     }

//     bool VoiceShouldRemove(int channel) {
//         return vol_env_[channel].GetState() == qwqdsp::AdsrEnvelope::State::Init;
//     }

//     // -------------------- parameters --------------------
//     float gliding_time_;
//     float attack;
//     float decay;
//     float sustain;
//     float release;

//     // ----------------------------------------
//     void Init(float fs) {
//         fs_ = fs;
//     }

//     void Process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_buffer) {
//         gliding_factor_ = qwqdsp::misc::ExpSmoother::ComputeSmoothFactor(gliding_time_, fs_, 2.0f);

//         size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());
//         float* left_ptr = buffer.getWritePointer(0);
//         float* right_ptr = buffer.getWritePointer(1);

//         int buffer_pos = 0;
//         for (auto midi : midi_buffer) {
//             auto message = midi.getMessage();
//             if (message.isNoteOnOrOff()) {
//                 ProcessRaw(left_ptr + buffer_pos, right_ptr + buffer_pos, static_cast<size_t>(midi.samplePosition - buffer_pos));
//                 buffer_pos = midi.samplePosition;

//                 if (message.isNoteOn()) {
//                     NoteOn(message.getNoteNumber(), message.getFloatVelocity());
//                 }
//                 else {
//                     NoteOff(message.getNoteNumber());
//                 }
//             }
//         }

//         ProcessRaw(left_ptr + buffer_pos, right_ptr + buffer_pos, num_samples - static_cast<size_t>(buffer_pos));
//     }

//     void ProcessRaw(float* left, float* right, size_t num_samples) noexcept {
//         std::fill_n(left, num_samples, 0.0f);
//         for (auto channel : active_channels_) {
//             ProcessAndAddInto(channel, left, num_samples);
//         }

//         RemoveDeadChannels();

//         std::copy_n(left, num_samples, right);
//     }

//     float gliding_factor_{};
//     float target_pitch_[kMaxPoly]{};
//     float current_pitch_[kMaxPoly]{};
//     float fs_;
//     qwqdsp::AdsrEnvelope vol_env_[kMaxPoly];
//     qwqdsp::oscillor::VicSineOsc oscs_[kMaxPoly];
// };
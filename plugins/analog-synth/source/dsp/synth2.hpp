#pragma once
#include <vector>

// void StopChannel(int channel);
// int FindMinVolChannel();
// void StartNewChannel(int channel, int note, float velocity, bool retrigger, float glide_begin_pitch, float target_pitch);
// float GetCurrentGlidingPitch(int channel);
// void ProcessAndAddInto(int channel, float* buffer, size_t num_samples);
// bool VoiceShouldRemove(int channel);
template<class Voice>
class AbstractSynth {
public:
    static constexpr size_t kMaxPoly = 8;

    void NoteOn(int note, float velocity) {
        if (free_channels_.empty()) {
            int allocate_channel = static_cast<Voice*>(this)->FindMinVolChannel();
            pending_notes_.push_back({playing_notes_[allocate_channel], playing_velocity_[allocate_channel]});

            StartNewChannel(allocate_channel, note, velocity, !is_legato_, static_cast<Voice*>(this)->GetCurrentGlidingPitch(active_channels_.back()), static_cast<float>(note));
            // if this stealing voice is triggerd, bring it to the next gliding pitch begin
            if (!is_legato_) {
                auto it = std::find(active_channels_.begin(), active_channels_.end(), allocate_channel);
                active_channels_.erase(it);
                active_channels_.push_back(allocate_channel);
            }
        }
        else {
            int allocated_channel = free_channels_.back();
            free_channels_.pop_back();

            float gliding_begin_pitch = active_channels_.empty() ? static_cast<float>(note) : static_cast<Voice*>(this)->GetCurrentGlidingPitch(active_channels_.back());
            static_cast<Voice*>(this)->StartNewChannel(allocated_channel, note, velocity, true, gliding_begin_pitch, static_cast<float>(note));

            playing_notes_[allocated_channel] = note;
            playing_velocity_[allocated_channel] = velocity;
            active_channels_.push_back(allocated_channel);
        }
    }

    void NoteOff(int note) {
        for (auto channel : active_channels_) {
            if (playing_notes_[channel] == note) {
                static_cast<Voice*>(this)->StopChannel(channel);
            }
        }

        auto it = std::remove_if(pending_notes_.begin(), pending_notes_.end(), [note](auto const& it_val) {
            return it_val.first == note;
        });
        pending_notes_.erase(it, pending_notes_.end());
    }

    void Process(float* buffer, size_t num_samples) {
        auto can_pending_add = std::min(free_channels_.size(), pending_notes_.size());
        for (size_t i = 0; i < can_pending_add; ++i) {
            auto data = pending_notes_.back();
            pending_notes_.pop_back();
            NoteOn(data.first, data.second);
        }

        std::fill_n(buffer, num_samples, 0.0f);
        for (auto channel : active_channels_) {
            static_cast<Voice*>(this)->ProcessAndAddInto(channel, buffer, num_samples);
        }

        std::vector<int> pending_remove_channels;
        for (auto channel : active_channels_) {
            if (static_cast<Voice*>(this)->VoiceShouldRemove(channel)) {
                pending_remove_channels.push_back(channel);
            }
        }

        for (auto channel : pending_remove_channels) {
            RemoveChannel(channel);
        }
    }

    void RemoveChannel(int channel) {
        auto it = std::remove(active_channels_.begin(), active_channels_.end(), channel);
        active_channels_.erase(it, active_channels_.end());
        free_channels_.push_back(channel);
    }

    std::vector<int> active_channels_;
    std::vector<int> free_channels_;
    int playing_notes_[kMaxPoly]{};
    float playing_velocity_[kMaxPoly]{};
    bool is_legato_{};

    std::vector<std::pair<int, float>> pending_notes_;
};
#pragma once
#include <vector>
#include <cstdint>

namespace analogsynth {
template<class T>
concept CVoice = requires (T s, uint32_t channel, int note, float velocity, bool retrigger, float glide_begin_pitch, float target_pitch) {
    // a note off event
    s.StopChannel(channel);
    // return `channel`
    {s.FindVoiceToSteal()} -> std::same_as<uint32_t>;
    // a note on event
    s.StartNewChannel(channel, note, velocity, retrigger, glide_begin_pitch, target_pitch);
    {s.GetCurrentGlidingPitch(channel)} -> std::same_as<float>;
    // is this voice is silence
    {s.VoiceShouldRemove(channel)} -> std::same_as<bool>;
};

template<class Voice>
class AbstractSynth {
protected:
    static constexpr size_t kMaxPoly = 8;

    AbstractSynth() {
        active_channels_.reserve(kMaxPoly);
        free_channels_.reserve(kMaxPoly);
        pending_notes_.reserve(128);
        SetNumVoices(kMaxPoly);
    }

    void NoteOn(int note, float velocity, bool join_pending = true) {
        if (free_channels_.empty()) {
            uint32_t allocate_channel = static_cast<Voice*>(this)->FindVoiceToSteal();
            if (join_pending && playing_notes_[allocate_channel] != note && playing_notes_[allocate_channel] != -1) {
                pending_notes_.push_back({playing_notes_[allocate_channel], playing_velocity_[allocate_channel]});
            }

            float gliding_begin_pitch = is_legato_ ? static_cast<Voice*>(this)->GetCurrentGlidingPitch(allocate_channel) : static_cast<Voice*>(this)->GetCurrentGlidingPitch(active_channels_.back());
            static_cast<Voice*>(this)->StartNewChannel(allocate_channel, note, velocity, !is_legato_, gliding_begin_pitch, static_cast<float>(note));
            playing_notes_[allocate_channel] = note;
            playing_velocity_[allocate_channel] = velocity;
            // if this stealing voice is triggerd, bring it to the next gliding pitch begin
            if (!is_legato_) {
                auto it = std::find(active_channels_.begin(), active_channels_.end(), allocate_channel);
                active_channels_.erase(it);
                active_channels_.push_back(allocate_channel);
            }
        }
        else {
            uint32_t allocated_channel = free_channels_.back();
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
                playing_notes_[channel] = -1;
            }
        }

        auto it = std::remove_if(pending_notes_.begin(), pending_notes_.end(), [note](auto const& it_val) {
            return it_val.first == note;
        });
        pending_notes_.erase(it, pending_notes_.end());

        if (!pending_notes_.empty()) {
            auto data = pending_notes_.back();
            pending_notes_.pop_back();
            NoteOn(data.first, data.second, false);
        }
    }

    void SetNumVoices(size_t num_voices) {
        num_max_voices_ = std::min(num_voices, kMaxPoly);
        pending_notes_.clear();
        free_channels_.clear();
        active_channels_.clear();
        for (size_t i = 0; i < num_max_voices_; ++i) {
            free_channels_.push_back(static_cast<uint32_t>(i));
        }
        std::fill_n(playing_notes_, kMaxPoly, -1);
    }

    void RemoveDeadChannels() {
        for (auto it = active_channels_.begin(); it != active_channels_.end();) {
            if (static_cast<Voice*>(this)->VoiceShouldRemove(*it)) {
                free_channels_.push_back(*it);
                it = active_channels_.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void AllNoteOff() noexcept {
        pending_notes_.clear();
        active_channels_.clear();
        free_channels_.clear();
        for (size_t i = 0; i < num_max_voices_; ++i) {
            free_channels_.push_back(static_cast<uint32_t>(i));
        }
        std::fill_n(playing_notes_, kMaxPoly, -1);
    }

    std::vector<uint32_t> active_channels_;
    bool is_legato_{};
private:
    size_t num_max_voices_{kMaxPoly};
    std::vector<uint32_t> free_channels_;
    int playing_notes_[kMaxPoly]{};
    float playing_velocity_[kMaxPoly]{};

    std::vector<std::pair<int, float>> pending_notes_;
};
}

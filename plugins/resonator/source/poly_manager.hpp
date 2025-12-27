#pragma once
#include "config.hpp"

struct Voice
{
    int voiceID;   // 复音的唯一 ID (0 到 kNumResonators - 1)
    int midiNote;  // 当前分配到的 MIDI 音高 (0-127)
    bool isActive; // 是否正在被使用 (NoteOn 状态)
    // 其他您需要的参数 (例如：音量、频率、内部滤波器状态等)
};

class PolyphonyManager {
public:
    PolyphonyManager() {
        initializeVoices();
    }

    /**
     * @brief 处理 NoteOn 事件，分配或替换复音。
     * @param note 要分配的 MIDI 音高。
     * @return 分配到的复音的 ID，如果找不到则返回 -1 (理论上不会发生)。
     */
    int noteOn(int note, bool round_robin) {
        if (round_robin) {
            // 测试循环位
            if (!voices[round_robin_].isActive) {
                int id = activateVoice(static_cast<int>(round_robin_), note);
                ++round_robin_;
                round_robin_ &= (kNumResonators - 1);
                return id;
            }
        }

        // 尝试找到一个空闲的复音
        for (size_t id = 0; id < kNumResonators; ++id) {
            if (!voices[id].isActive) {
                // 找到空闲复音：直接分配
                return activateVoice(static_cast<int>(id), note);
            }
        }

        // --- 所有复音都被占用：FIFO 替换最旧的 ---
        
        // 获取最旧的复音 ID (deque 的头部)
        int oldestVoiceID = activationOrder.front();
        activationOrder.pop_front();

        // 激活新音高并返回 ID
        return activateVoice(oldestVoiceID, note);
    }

    /**
     * @brief 处理 NoteOff 事件，释放对应的复音。
     * @param note 要释放的 MIDI 音高。
     * @return 返回被关闭的id，如果没有则是kNumResonators
     */
    int noteOff(int note) {
        // 找到所有分配给该音高的复音并释放
        for (size_t id = 0; id < kNumResonators; ++id) {
            if (voices[id].isActive && voices[id].midiNote == note) {
                deactivateVoice(static_cast<int>(id));
                // 注意：如果有多个复音分配到同一个音高，这里只会释放找到的第一个。
                // 如果需要支持单音高多复音，则可以继续循环或使用更复杂的映射。
                return static_cast<int>(id);
            }
        }
        return kNumResonators;
    }
    
    // -----------------------------------------------------------
    // 调试/查询方法
    // -----------------------------------------------------------
    const Voice& getVoice(size_t id) const {
        return voices[id];
    }

    void initializeVoices() {
        for (size_t i = 0; i < kNumResonators; ++i) {
            voices[i].voiceID = static_cast<int>(i);
            voices[i].midiNote = -1;
            voices[i].isActive = false;
        }
        round_robin_ = 0;
    }

private:
    std::array<Voice, kNumResonators> voices;           // 存储所有复音实例
    std::deque<int> activationOrder;     // 存储正在使用复音的 ID，用于 FIFO 替换
    size_t round_robin_{};

    int activateVoice(int id, int note) {
        voices[static_cast<size_t>(id)].midiNote = note;
        voices[static_cast<size_t>(id)].isActive = true;
        
        // 将此复音 ID 添加到激活队列的尾部，表示它现在是“最新”的
        activationOrder.push_back(id);
        return id;
    }

    void deactivateVoice(int id) {
        voices[static_cast<size_t>(id)].isActive = false;
        voices[static_cast<size_t>(id)].midiNote = -1;

        // 从激活队列中移除此 ID，因为它不再是“激活”状态
        // 使用 std::remove_if + erase idiom
        activationOrder.erase(
            std::remove(activationOrder.begin(), activationOrder.end(), id),
            activationOrder.end()
        );
    }
};

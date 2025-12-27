#pragma once
#include <semaphore>
#include <juce_core/juce_core.h>

namespace pluginshared {
class UpdateData {
public:
    static constexpr int kNetworkTimeout = 500; // ms
    static constexpr auto kReleasePageURL = "https://github.com/a5632645/dev-plugins/releases";
    static constexpr auto kReleaseJsonFile = "https://raw.githubusercontent.com/a5632645/dev-plugins/refs/heads/main/release.json";

    UpdateData() {
        is_thread_run_ = update_thread_.startThread();
    }

    ~UpdateData() {
        if (is_thread_run_) {
            update_thread_.Quit();
        }
    }

    void BeginCheck() {
        update_thread_.BeginCheck();
    }

    bool IsComplete() const {
        return update_thread_.is_complete_;
    }

    juce::String GetUpdateMessage() const {
        return update_thread_.GetUpdateMessage();
    }

    juce::String GetButtonLabel() const {
        return update_thread_.GetButtonLabel();
    }

    bool HaveNewVersion() const {
        return update_thread_.have_new_version_;
    }
private:
    class UpdateThread : public juce::Thread {
    public:
        UpdateThread()
            : juce::Thread("version check")
        {}
    
        void run() override {
            for (;;) {
                get_sem_.acquire();
                if (threadShouldExit()) {
                    return;
                }
    
                // get release json from github
                juce::URL::InputStreamOptions op{juce::URL::ParameterHandling::inAddress};
                auto stream = juce::URL{kReleaseJsonFile}
                    .createInputStream(op.withConnectionTimeoutMs(kNetworkTimeout));
        
                if (!stream) {
                    SetUpdateMessage("network error");
                    SetButtonLabel("ok");
                    is_complete_ = true;
                    continue;
                }
        
                auto version_string = stream->readEntireStreamAsString();
                auto json = juce::JSON::fromString(version_string);
                if (!json.isArray()) {
                    SetUpdateMessage("payload error");
                    SetButtonLabel("ok");
                    is_complete_ = true;
                    continue;
                }
        
                auto* array = json.getArray();
                if (!array) {
                    SetUpdateMessage("payload error");
                    SetButtonLabel("ok");
                    is_complete_ = true;
                    continue;
                }
        
                bool find_data = false;
                for (auto const& plugin : *array) {
                    auto plugin_name = plugin.getProperty("name", "");
                    if (!plugin_name.isString()) {
                        continue;
                    }
                    if (!plugin_name.toString().equalsIgnoreCase(JucePlugin_Name)) {
                        continue;
                    }
        
                    auto version = plugin.getProperty("version", JucePlugin_VersionString);
                    if (!version.isString()) {
                        SetUpdateMessage("payload error");
                        SetButtonLabel("ok");
                        break;
                    }
                    if (version.toString().equalsIgnoreCase(JucePlugin_VersionString)) {
                        SetUpdateMessage("you are using the newest plugin");
                        SetButtonLabel("ok");
                        find_data = true;
                        break;
                    }
                    else {
                        auto changelog = plugin.getProperty("changelog", "").toString();
                        juce::String s;
                        s << "new version avaliable: " << version.toString() << "\n";
                        s << "chanelog:\n" << changelog;
                        SetUpdateMessage(s);
                        SetButtonLabel("ok");
                        have_new_version_ = true;    
                        find_data = true;
                    }
                }
        
                if (!find_data) {
                    SetUpdateMessage("payload error");
                    SetButtonLabel("ok");
                }
                is_complete_ = true;
            }
        }
    
        void BeginCheck() {
            is_complete_ = false;
            have_new_version_ = false;
            get_sem_.release();
            {
                juce::ScopedLock lock{data_lock_};
                update_message_.clear();
                button_text_.clear();
            }
        }
    
        void Quit() {
            get_sem_.release();
            stopThread(-1);
        }

        juce::String GetUpdateMessage() const {
            juce::ScopedLock lock{data_lock_};
            return update_message_;
        }

        juce::String GetButtonLabel() const {
            juce::ScopedLock lock{data_lock_};
            return button_text_;
        }
    private:
        void SetUpdateMessage(const juce::String& message) {
            juce::ScopedLock lock{data_lock_};
            update_message_ = message;
        }

        void SetButtonLabel(const juce::String& label) {
            juce::ScopedLock lock{data_lock_};
            button_text_ = label;
        }

        friend class UpdateData;
        std::atomic<bool> is_complete_{false};
        std::atomic<bool> have_new_version_{false};
        std::binary_semaphore get_sem_{0};
        juce::CriticalSection data_lock_;
        juce::String update_message_;
        juce::String button_text_;
    };

    UpdateThread update_thread_;
    bool is_thread_run_{};
};
}

#pragma once

#include <vector>

namespace mana::utli {
template<class Listener>
class ListenerList {
public:
    void AddListener(Listener* l) {
        if (std::ranges::find(listeners_, l) == listeners_.cend())
            listeners_.push_back(l);
    }

    void RemoveListener(Listener* l) {
        auto it = std::ranges::find(listeners_, l);
        if (it != listeners_.cend())
            listeners_.erase(it);
    }

    template<class Func, class... Args> requires std::invocable<Func, Listener*, Args...>
    void CallListener(Func&& func, Args&&... args) {
        for (auto* l : listeners_) {
            std::invoke(std::forward<Func>(func), l, std::forward<Args>(args)...);
        }
    }
private:
    std::vector<Listener*> listeners_;
};
}
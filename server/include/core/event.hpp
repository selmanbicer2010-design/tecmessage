#pragma once
#include <functional>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "core/util.hpp"

namespace event{

template <typename... Args>
class ordered_event
{
    using Callback = std::function<void(const Args&...)>;
    struct CallbackInfo
    {
        int id = -1;
        Callback callback;
        int order = 0;
        bool active = true;
        bool once = false;
        bool destroy = false;
    };
public:
    bool active = true;

    int next()
    {
        return currentId_ + 1;
    }

    int connect(Callback callback, int order = 0)
    {
        if (!callback) { return -1; }
        CallbackInfo info;
        info.id = ++currentId_;
        info.order = order;
        info.callback = callback;
        callbacks_.push_back(info);
        return currentId_;
    }

    int once(Callback callback, int order = 0)
    {
        if (!callback) { return -1; }
        CallbackInfo info;
        info.id = ++currentId_;
        info.order = order;
        info.callback = callback;
        info.once = true;
        callbacks_.push_back(info);
        return currentId_;
    }

    void disconnect(int id)
    {
        if (!firing_)
        {
            callbacks_.erase(
                std::remove_if(callbacks_.begin(), callbacks_.end(),
                    [id](const CallbackInfo& info) { return info.id == id; }),
                callbacks_.end()
            );
        }
        if (firing_)
        {
            for(auto& info : callbacks_)
            { if (info.id == id) { info.destroy = true; } }
        }
    }

    void status(int id, bool activity)
    {
        for (CallbackInfo& info : callbacks_)
        {
            if (info.id == id)
            { info.active = activity; break; }
        }
    }

    void fire(const Args&... args)
    {
        if (!active) { return; }
        std::sort(callbacks_.begin(), callbacks_.end(), [](const CallbackInfo& cbA, const CallbackInfo& cbB){
            if (cbA.order != cbB.order)
            {
                return cbA.order < cbB.order;
            }
            return cbA.id < cbB.id;
        });
        firing_ = true;
        int size = callbacks_.size();
        for (int i{0}; i < size; i++)
        {
            if (callbacks_[i].active)
            {callbacks_[i].callback(args...);}
            if (callbacks_[i].once)
            {callbacks_[i].destroy = true;}
        }
        firing_ = false;
        callbacks_.erase(
                std::remove_if(callbacks_.begin(), callbacks_.end(),
                    [](const CallbackInfo& info) { return info.destroy == true; }),
                callbacks_.end()
            );
    }

private:
    std::vector<CallbackInfo> callbacks_;
    int currentId_ = -1;
    bool firing_ = false;

};

template <typename... Args>
class basic_event
{
private:
    using Callback = std::function<void(const Args&...)>;
    std::unordered_map<uint32_t, Callback> callbacks_;
    std::unordered_map<uint32_t, Callback> callbacksOnce_;
    uint32_t currentId_{0};
public:
    int32_t connect(Callback cb)
    {
        int32_t id = currentId_++;
        callbacks_.insert({id, cb});
        return id;
    }

    int32_t once(Callback cb)
    {
        int32_t id = currentId_++;
        callbacksOnce_.insert({id, cb});
        return id;
    }

    void disconnect(uint32_t cbid)
    {
        callbacks_.erase(cbid);
        callbacksOnce_.erase(cbid);
    }

    void fire(const Args&... args)
    {
        auto copy = callbacks_;
        auto copyonce = callbacksOnce_;
        callbacksOnce_.clear();
        for (auto& [cbid, cb] : copy)
        {
            cb(args...);
        }
        for (auto& [cbid, cb] : copyonce)
        {
            cb(args...);
        }
    }

};

template <typename... args_pack>
class async_event {
private:
    using func = std::function<void(const args_pack&...)>;
    util::unordered_vector<func> callbacks{};
    mutable std::mutex mtx;
public:
    async_event() = default;
    async_event(const async_event& other) {
        *this = other;
    }
    async_event(async_event&& other) {
        *this = std::move(other);
    }
    async_event& operator=(const async_event& other) {
        if (this == &other) return *this;
        std::scoped_lock lock{mtx, other.mtx};
        this->callbacks = other.callbacks;
        return *this;
    }
    async_event& operator=(async_event&& other) {
        if (this == &other) return *this;
        std::scoped_lock lock{mtx, other.mtx};
        this->callbacks = std::move(other.callbacks);
        return *this;
    }
    int32_t connect(func&& callback) {
        std::lock_guard<std::mutex> lg{mtx};
        return callbacks.push_back(std::move(callback));
    }

    int32_t once(func&& callback) {
        std::lock_guard<std::mutex> lg{mtx};
        int32_t handle = callbacks.next_handle();
        return callbacks.push_back(std::move([this, callback = std::move(callback), handle](const args_pack&... args) mutable { callbacks.erase(handle); callback(args...); }));
    }

    void disconnect(int32_t key) {
        std::lock_guard<std::mutex> lg{mtx};
        callbacks.erase(key);
    }

    void fire(const args_pack&... args) {
        util::unordered_vector<func> copy;
        {
            std::lock_guard<std::mutex> lg{mtx};
            copy = callbacks;
        }
        for (auto& fn : copy) {
            fn(args...);
        }
    }
};

template <typename... Args>
class signal
{
private:
    using Callback = std::function<void(const Args&...)>;
    bool once_ = false;
    Callback callback_ = [](auto&...){};
public:
    void connect(Callback callback)
    {
        callback_ = std::move(callback);
        once_ = false;
    }
    void once(Callback callback)
    {
        callback_ = std::move(callback);
        once_ = true;
    }
    void disconnect()
    {
        callback_ = [](auto&...){};
        once_ = false;
    }
    void fire(const Args&... args)
    {
        callback_(args...);
        if (once_)
        {
            disconnect();
        }
    }
};

template<typename T_Event, typename U_Event, typename Func, typename Conditional> // [](auto&&...){return true;}
int pconnectif(T_Event& eventToConnect, U_Event& eventToDisconnectAt, Conditional&& conditionalFunc, Func&& callback)
{
    int callbackId  = eventToConnect.connect(callback);
    int disconnectId = eventToDisconnectAt.next();
    eventToDisconnectAt.connect([&eventToConnect, &eventToDisconnectAt, conditionalFunc, callbackId, disconnectId](auto&& args...){
        if (!conditionalFunc(args)) return;
        eventToConnect.disconnect(callbackId);
        eventToDisconnectAt.disconnect(disconnectId);
    });
    return callbackId;
}

template<typename T_Event, typename U_Event, typename Func>
int pconnect(T_Event& eventToConnect, U_Event& eventToDisconnectAt, Func&& callback)
{
    int callbackId  = eventToConnect.connect(callback);
    eventToDisconnectAt.once([&eventToConnect, callbackId](auto&&...){eventToConnect.disconnect(callbackId);});
    return callbackId;
}

}

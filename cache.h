#pragma once
#ifndef CACHE_H
#define CACHE_H
#include "base64.h"
#include <unordered_map>
#include <mutex>
#include <Windows.h>
#include "ThreadSafeQueue.h"
#include <optional>

class Cache {
public:
    // 获取 Cache 实例
    static Cache& getInstance() {
        static Cache instance;
        return instance;
    }

    // 设置缓存数据
    void set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[key] = value;
    }

    // 获取缓存数据，如果不存在则返回空字符串
    std::string get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        else {
            return "";  // 返回空字符串作为默认值
        }
    }

    // 获取缓存数据，如果不存在则返回提供的默认值
    std::string get(const std::string& key, const std::string& default_value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        else {
            return default_value;  // 返回提供的默认值
        }
    }

    // 删除缓存数据
    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }

    // 清空缓存数据
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }

    // 更新缓存数据，如果键不存在则插入
    void update(const std::string& key, const std::string& value) {
        set(key, value);  // 在这个简单的实现中，更新操作与设置操作是相同的
    }

    // 设置和获取 base_addr_
    void setBaseAddr(UINT64 baseAddr) {
        base_addr_ = baseAddr;
    }

    UINT64 getBaseAddr() const {
        return base_addr_;
    }
    // 设置和获取 global_index_
    void setGlobalIndex(INT64 aaaa) {
        aaa = aaaa;
    }

    INT64 getGlobalIndex() const {
        return aaa;
    }

    // 队列操作方法
    void enqueueMessage(const std::string& message) {
        queue_.push(message);
    }

    bool popMessage(std::string& value) {
        return queue_.try_pop(value);
    }

private:
    // 禁止拷贝构造和赋值操作
    Cache() {
        // 获取模块基地址
        base_addr_ = (UINT64)GetModuleHandleA("WeChatWin.dll");

        gdface::mt::threadsafe_queue<std::string> queue;
    }
    ~Cache() = default;
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    mutable std::mutex mutex_;  // 将互斥量声明为可变，以允许在 const 方法中加锁
    std::unordered_map<std::string, std::string> cache_;
    UINT64 base_addr_;
    gdface::mt::threadsafe_queue<std::string> queue_;  // 线程安全队列
    int global_index_; // 全局索引值
    INT64 aaa;
};

#endif // CACHE_H

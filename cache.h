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
    // ��ȡ Cache ʵ��
    static Cache& getInstance() {
        static Cache instance;
        return instance;
    }

    // ���û�������
    void set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[key] = value;
    }

    // ��ȡ�������ݣ�����������򷵻ؿ��ַ���
    std::string get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        else {
            return "";  // ���ؿ��ַ�����ΪĬ��ֵ
        }
    }

    // ��ȡ�������ݣ�����������򷵻��ṩ��Ĭ��ֵ
    std::string get(const std::string& key, const std::string& default_value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        else {
            return default_value;  // �����ṩ��Ĭ��ֵ
        }
    }

    // ɾ����������
    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }

    // ��ջ�������
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }

    // ���»������ݣ�����������������
    void update(const std::string& key, const std::string& value) {
        set(key, value);  // ������򵥵�ʵ���У����²��������ò�������ͬ��
    }

    // ���úͻ�ȡ base_addr_
    void setBaseAddr(UINT64 baseAddr) {
        base_addr_ = baseAddr;
    }

    UINT64 getBaseAddr() const {
        return base_addr_;
    }
    // ���úͻ�ȡ global_index_
    void setGlobalIndex(INT64 aaaa) {
        aaa = aaaa;
    }

    INT64 getGlobalIndex() const {
        return aaa;
    }

    // ���в�������
    void enqueueMessage(const std::string& message) {
        queue_.push(message);
    }

    bool popMessage(std::string& value) {
        return queue_.try_pop(value);
    }

private:
    // ��ֹ��������͸�ֵ����
    Cache() {
        // ��ȡģ�����ַ
        base_addr_ = (UINT64)GetModuleHandleA("WeChatWin.dll");

        gdface::mt::threadsafe_queue<std::string> queue;
    }
    ~Cache() = default;
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    mutable std::mutex mutex_;  // ������������Ϊ�ɱ䣬�������� const �����м���
    std::unordered_map<std::string, std::string> cache_;
    UINT64 base_addr_;
    gdface::mt::threadsafe_queue<std::string> queue_;  // �̰߳�ȫ����
    int global_index_; // ȫ������ֵ
    INT64 aaa;
};

#endif // CACHE_H

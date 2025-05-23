/*
 * @Description: Copyright Xiao
 * @Autor: Xjj
 */

#ifndef CRING_BUFF_H
#define CRING_BUFF_H

#include "spin_lock.h"
#include <iostream>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <atomic>
#include <memory>

class CRingBuff {
public:
    CRingBuff(std::uint64_t size);
    ~CRingBuff();

    void* operator new(size_t size) {
        return aligned_alloc(64, size); // 使用 64 字节对齐分配
    }
    void operator delete(void* ptr) {
        free(ptr);                      // 使用对应的释放方式
    }

public:
    std::uint64_t GetLen();
    std::uint64_t GetCap();
    bool PutData(const std::uint8_t *data, std::uint64_t len);
    bool GetData(std::uint8_t *outData, std::uint64_t & len);

private:
    static std::uint64_t NextPowerOfTwo(std::uint64_t size);

private:
    alignas(64) std::atomic<std::uint64_t>   m_head;  // 强制 64 字节对齐
    alignas(64) std::atomic<std::uint64_t>   m_tail;  
    alignas(64) std::uint64_t                m_mask;
    alignas(64) bool                         m_disposed;               
    alignas(64) std::uint8_t*                m_data;
    // alignas(64) std::mutex                   m_mtxOperate;
    alignas(64) SpinLock                  m_spinLock; 
};

CRingBuff::CRingBuff(std::uint64_t size) : m_head(0), m_tail(0), m_disposed(false)
{
    size = NextPowerOfTwo(size);
    m_data = new std::uint8_t[size];
    memset(m_data, 0, size);
    m_mask = size - 1;
}

CRingBuff::~CRingBuff()
{
    m_disposed = true;
    delete[] m_data;
}

std::uint64_t CRingBuff::NextPowerOfTwo(std::uint64_t size) {
    size--;
	size |= size >> 1;
	size |= size >> 2;
	size |= size >> 4;
	size |= size >> 8;
	size |= size >> 16;
	size |= size >> 32;
	size++;
	return size;
} 

std::uint64_t CRingBuff::GetLen() {
    // std::lock_guard<std::mutex> lock(m_mtxOperate);
    // m_spinLock.lock();
    uint64_t head = m_head.load(std::memory_order_acquire);
    uint64_t tail = m_tail.load(std::memory_order_acquire);
    // m_spinLock.unlock();
    
    std::cout << "tail : " << tail << " head : " << head << std::endl;
    return  tail - head;
}

std::uint64_t CRingBuff::GetCap() {
    return m_mask + 1;
}

bool CRingBuff::PutData(const std::uint8_t *data, const std::uint64_t len) {
    const std::uint64_t head = m_head.load(std::memory_order_acquire);
    const std::uint64_t tail = m_tail.load(std::memory_order_acquire);
    if (head > tail || m_disposed || len == 0 || data == nullptr) {
        std::cout << m_disposed << " P " << len <<std::endl;
        return false;
    }
    const std::uint64_t capacity = m_mask + 1;
    // std::lock_guard<std::mutex> lock(m_mtxOperate);
    // m_spinLock.lock();
    // const std::uint64_t head = m_head.load(std::memory_order_acquire);
    // const std::uint64_t tail = m_tail.load(std::memory_order_acquire);
    const std::uint64_t    free_space = capacity - (tail - head);
    if (len > free_space) {
       // std::cout << free_space << " P " << head << " P " << tail <<std::endl;
        return false;
    }
    // m_tail = tail + len;
    m_tail.store(tail + len, std::memory_order_release);
    // m_spinLock.unlock();
    std::uint64_t write_pos = tail & m_mask;
    if (write_pos + len <= capacity) {
        memcpy(m_data + write_pos, data, len);
    } else {
        std::uint64_t first_part = capacity - write_pos;
        memcpy(m_data + write_pos, data, first_part);
        memcpy(m_data, data + first_part, len - first_part);
    }

    return true;
}

bool CRingBuff::GetData(std::uint8_t *outData, std::uint64_t & len) {
    const uint64_t head = m_head.load(std::memory_order_acquire);
    const uint64_t tail = m_tail.load(std::memory_order_acquire);
    if (head > tail || m_disposed || len == 0) {
        std::cout << m_disposed << " C " << len <<std::endl;
        return false;
    }
    const uint64_t capacity = m_mask + 1;    
    // std::lock_guard<std::mutex> lock(m_mtxOperate);
    // m_spinLock.lock();
    // const uint64_t head = m_head.load(std::memory_order_acquire);
    // const uint64_t tail = m_tail.load(std::memory_order_acquire);
    const uint64_t availData = (tail - head);
    if (availData < len) {
        len = 0;
       // std::cout << availData << " C " << head << " C " << tail <<std::endl;
        return false;
    }
    // m_head = head + len;
    m_head.store(head + len, std::memory_order_release);
    // std::cout << "m_head" << m_head << std::endl;
    // m_spinLock.unlock();
    uint64_t read_pos = head & m_mask;
    if (read_pos + len <= capacity) {
        memcpy(outData, m_data + read_pos, len);
    } else {
        uint64_t first_part = capacity - read_pos;
        memcpy(outData, m_data + read_pos, first_part);
        memcpy(outData + first_part, m_data, len - first_part);
    }
    return true;
}

#endif //CRING_BUFF_H
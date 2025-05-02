/*
 * @Description: Copyright Xiao
 * @Autor: Xjj
 */

#include "ring_buff.h"
#include <atomic>
#include <thread>

#define EPOCH 1024 * 1024 * 16

CRingBuff* buff = nullptr;
std::atomic<std::uint64_t> countProduce{0};
std::atomic<std::uint64_t> countConsume{0};

void Producer() {
    auto start = std::chrono::high_resolution_clock::now(); 
    std::uint8_t data[32] = "qwertyuiopasdfghjklzxcvbnm12345";
    for (int i = 0; i < EPOCH; ++i) {
        while (!buff->PutData(data, sizeof(data))) {
            std::cout << "P yield: " << std::endl;
            std::this_thread::yield();    
        }
        countProduce += sizeof(data);
        // std::cout << "P : " << countProduce << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double duration_sec = duration_ms.count() / 1000.0;
    double total_mb = EPOCH * 32 / (1024.0 * 1024.0);
    double mb_per_sec = total_mb / duration_sec;

    // std::cout << std::fixed << std::setprecision(2);
    std::cout << "Producer: " 
              << duration_ms.count() << " ms | "
              << total_mb << " MB | "
              << mb_per_sec << " MB/s\n";
}

void Consumer() {
    auto start = std::chrono::high_resolution_clock::now();
    std::uint8_t data[32];
    while (countConsume.load(std::memory_order_acquire) < 32 * EPOCH) {
        std::uint64_t len = sizeof(data);
        if (buff->GetData(data, len)) {
            countConsume += len;
           // std::cout << "C : " << countConsume << std::endl;
        } else {
            std::cout << "C yield: " << std::endl;
            std::this_thread::yield();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double duration_sec = duration_ms.count() / 1000.0;
    double total_mb = EPOCH * 32 / (1024.0 * 1024.0);
    double mb_per_sec = total_mb / duration_sec;

    std::cout << "Consumer: " 
              << duration_ms.count() << " ms | "
              << total_mb << " MB | "
              << mb_per_sec << " MB/s\n";
}

int main() {
    buff = new CRingBuff(1024 * 16);

    std::thread t1(Producer);
    std::thread t2(Consumer);

    if(t1.joinable()) {
        t1.join();
    }
    if(t2.joinable()) {
        t2.join();
    }

    delete buff;
    std::cout << "done" << std::endl;
    return 0;
}

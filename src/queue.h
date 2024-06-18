#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>

namespace qalarm {

template <typename T>
class MPSCQueue {
public:
    explicit MPSCQueue(size_t capacity = kDefaultCapacity)
        : capacity_(capacity < kDefaultCapacity ? kDefaultCapacity : capacity)
        , head_(0)
        , tail_(0) {
        size_t space = capacity * sizeof(Slot) + kCacheLineSize - 1;
        buffer_ = malloc(space);
        if (buffer_ == nullptr) {
            throw std::bad_alloc();
        }

        void *buffer = buffer_;
        slots_ = reinterpret_cast<Slot *>(std::align(kCacheLineSize, capacity * sizeof(Slot), buffer, space));

        if (slots_ == nullptr) {
            free(buffer_);
            throw std::bad_alloc();
        }

        for (size_t i = 0; i < capacity_; ++i) {
            new (&slots_[i]) Slot();
        }

        static_assert(sizeof(MPSCQueue<T>) % kCacheLineSize == 0,
                      "MPSCQueue<T> size must be a multiple of cache line size to "
                      "prevent false sharing between adjacent queues");
        static_assert(sizeof(Slot) % kCacheLineSize == 0,
                      "Slot size must be a multiple of cache line size to prevent "
                      "false sharing between adjacent slots");
        assert(reinterpret_cast<size_t>(slots_) % kCacheLineSize == 0 &&
               "slots_ array must be aligned to cache line size to prevent false "
               "sharing between adjacent slots");
        assert(reinterpret_cast<char *>(&tail_) - reinterpret_cast<char *>(&head_) >=
                   static_cast<std::ptrdiff_t>(kCacheLineSize) &&
               "head and tail must be a cache line apart to prevent false sharing");
    }

    ~MPSCQueue() {
        for (size_t i = 0; i < capacity_; ++i) {
            slots_[i].~Slot();
        }
        free(buffer_);
    }

    MPSCQueue(const MPSCQueue &) = delete;
    MPSCQueue operator=(const MPSCQueue &) = delete;

public:
    void Push(const T &v) noexcept(std::is_nothrow_copy_constructible<T>::value) {
        static_assert(std::is_copy_constructible<T>::value, "T must be copy constructible");
        Emplace(v);
    }

    template <typename P, typename = typename std::enable_if<std::is_constructible<T, P &&>::value>::type>
    void Push(P &&v) noexcept(std::is_nothrow_constructible<T, P &&>::value) {
        Emplace(std::forward<P>(v));
    }

    bool TryPush(const T &v) noexcept(std::is_nothrow_copy_constructible<T>::value) {
        static_assert(std::is_copy_constructible<T>::value, "T must be copy constructible");
        return TryEmplace(v);
    }

    template <typename P, typename = typename std::enable_if<std::is_constructible<T, P &&>::value>::type>
    bool TryPush(P &&v) noexcept(std::is_nothrow_constructible<T, P &&>::value) {
        return TryEmplace(std::forward<P>(v));
    }

    template <typename... Args>
    void Emplace(Args &&... args) noexcept(std::is_nothrow_constructible<T, Args &&...>::value) {
        static_assert(std::is_constructible<T, Args &&...>::value, "T must be constructible with Args&&...");
        size_t head = 0;
        size_t nextHead = 0;
        do {
            head = head_.load(std::memory_order_acquire);
            nextHead = (head + 1) % capacity_;
        } while (nextHead == tail_.load(std::memory_order_acquire) || !head_.compare_exchange_weak(head, nextHead));

        slots_[head].Construct(std::forward<Args>(args)...);
        slots_[head].ready.store(true, std::memory_order_release);
    }

    template <typename... Args>
    bool TryEmplace(Args &&... args) noexcept(std::is_nothrow_constructible<T, Args &&...>::value) {
        static_assert(std::is_constructible<T, Args &&...>::value, "T must be constructible with Args&&...");
        size_t head = 0;
        size_t nextHead = 0;
        do {
            head = head_.load(std::memory_order_acquire);
            nextHead = (head + 1) % capacity_;
            if (nextHead == tail_.load(std::memory_order_acquire)) {
                return false;
            }
        } while (!head_.compare_exchange_weak(head, nextHead));

        slots_[head].Construct(std::forward<Args>(args)...);
        slots_[head].ready.store(true, std::memory_order_release);
        return true;
    }

    void Pop(T &v) noexcept {
        static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");
        auto const tail = tail_.load(std::memory_order_acquire);
        while (head_.load(std::memory_order_acquire) == tail || !slots_[tail].ready.load(std::memory_order_acquire)) {
        }
        v = slots_[tail].Move();
        slots_[tail].ready.store(false, std::memory_order_release);
        auto nextTail = (tail + 1) % capacity_;
        tail_.store(nextTail, std::memory_order_release);
    }

    bool TryPop(T &v) noexcept {
        static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");
        auto const tail = tail_.load(std::memory_order_acquire);
        if (head_.load(std::memory_order_acquire) == tail) {
            return false;
        }
        if (!slots_[tail].ready.load(std::memory_order_acquire)) {
            return false;
        }
        v = slots_[tail].Move();
        slots_[tail].ready.store(false, std::memory_order_release);
        auto nextTail = (tail + 1) % capacity_;
        tail_.store(nextTail, std::memory_order_release);
        return true;
    }

    T *Front() noexcept {
        auto const tail = tail_.load(std::memory_order_acquire);
        if (head_.load(std::memory_order_acquire) == tail) {
            return nullptr;
        }
        const auto slot = slots_[tail];
        if (!slot.ready.load(std::memory_order_acquire)) {
            return nullptr;
        }
        return &slots_[tail];
    }

    void Pop() noexcept {
        static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");
        auto const tail = tail_.load(std::memory_order_acquire);
        assert(head_.load(std::memory_order_acquire) != tail);
        assert(slots_[tail].ready.load(std::memory_order_acquire));
        slots_[tail].Destruct();
        slots_[tail].ready.store(false, std::memory_order_release);
        auto nextTail = (tail + 1) % capacity_;
        tail_.store(nextTail, std::memory_order_release);
    }

    std::vector<T> TryPopBulk() noexcept {
        static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");
        auto const tail = tail_.load(std::memory_order_acquire);
        auto const head = head_.load(std::memory_order_acquire);
        if (head == tail) {
            return std::vector<T>();
        }
        auto const max = (head + capacity_ - tail) % capacity_;
        std::vector<T> bulk;
        bulk.reserve(max);
        for (size_t offset = 0; offset < max; ++offset) {
            const auto index = (tail + offset) % capacity_;
            if (slots_[index].ready.load(std::memory_order_acquire)) {
                bulk.emplace_back(slots_[index].Move());
                slots_[index].ready.store(false, std::memory_order_release);
            } else {
                break;
            }
        }
        auto nextTail = (tail + bulk.size()) % capacity_;
        tail_.store(nextTail, std::memory_order_release);
        return bulk;
    }

private:
    static_assert(std::is_nothrow_copy_assignable<T>::value || std::is_nothrow_move_assignable<T>::value,
                  "T must be nothrow copy or move assignable");
    static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");

private:
    static constexpr size_t kDefaultCapacity = 256;
    static constexpr size_t kCacheLineSize = 128;

private:
    struct Slot {
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;

    public:
        Slot()
            : ready(false) {}

        ~Slot() noexcept {
            if (ready.load(std::memory_order_acquire)) {
                Destruct();
            }
        }

    public:
        template <typename... Args>
        void Construct(Args &&... args) noexcept {
            static_assert(std::is_nothrow_constructible<T, Args &&...>::value,
                          "T must be nothrow constructible with Args&&...");
            new (&storage) T(std::forward<Args>(args)...);
        }

        void Destruct() noexcept {
            static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");
            reinterpret_cast<T *>(&storage)->~T();
        }

        T &&Move() noexcept {
            return reinterpret_cast<T &&>(storage);
        }

    public:
        // odd->available even->empty
        // Align to avoid false sharing between adjacent slots
        alignas(kCacheLineSize) std::atomic<bool> ready;
    };

private:
    const size_t capacity_;
    Slot *slots_;
    void *buffer_;

    // Align to avoid false sharing between head_ and tail_
    alignas(kCacheLineSize) std::atomic<size_t> head_;
    alignas(kCacheLineSize) std::atomic<size_t> tail_;
};

} // namespace qalarm

#pragma once
#include <limits>
#include <memory>

namespace qwqdsp_psimd {
/**
 * @brief 内存对齐分配器
 * @ref https://github.com/AutoPas/AutoPas/blob/0ea349ddb6c6048e1d00b753864e2c6fedd5b74a/src/autopas/utils/AlignedAllocator.h#L29
 * @note 添加了MSVC和其他编译器
 */
template <class T, size_t Alignment>
class AlignedAllocator {
public:
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = size_t;

    template <class U>
    struct rebind {
        using other = AlignedAllocator<U, Alignment>;
    };

    AlignedAllocator() = default;

    template <class U>
    AlignedAllocator(const AlignedAllocator<U, Alignment> &) {}

    ~AlignedAllocator() = default;

    size_t max_size() const noexcept {
        return (std::numeric_limits<size_t>::max() - size_t(Alignment)) / sizeof(T);
    }

    T *allocate(std::size_t n) {
        void* ptr{};
#ifdef _MSC_VER
        ptr = _aligned_malloc(n * sizeof(T), Alignment);
#else
        ptr = std::aligned_alloc(Alignment, n * sizeof(T));
#endif
        if (ptr == nullptr) {
            throw std::bad_alloc{};
        }
        return reinterpret_cast<T*>(ptr);
    }

    void deallocate(T *ptr, std::size_t /*n*/) {
#ifdef _MSC_VER
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    template <class U, class... Args>
    void construct(U *p, Args &&...args) {
#if __cplusplus > 201703L
        std::construct_at(p, std::forward<Args...>(args)...);
#else
        ::new (static_cast<void*>(p)) U(std::forward<Args...>(args)...);
#endif
    }

    template <class U>
    void destroy(U *p) {
        p->~U();
    }
};
}

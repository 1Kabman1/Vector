#pragma once

#include <cassert>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <new>
#include <utility>

template<typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
            : buffer_(Allocate(capacity)), capacity_(capacity) {
    }

    ~RawMemory();

    RawMemory(const RawMemory &) = delete;

    RawMemory &operator=(const RawMemory &rhs) = delete;

    RawMemory(RawMemory &&other) noexcept: buffer_(std::exchange(other.buffer_, nullptr)),
                                           capacity_(std::exchange(other.capacity_, 0)) {}

    RawMemory &operator=(RawMemory &&rhs) noexcept {

        if (this != &rhs) {
            buffer_ = std::move(rhs.buffer_);
            capacity_ = std::move(rhs.capacity_);
            rhs.buffer_ = nullptr;
            rhs.capacity_ = 0;
        }

        return *this;
    }


    T *operator+(size_t offset) noexcept;

    const T *operator+(size_t offset) const noexcept;

    const T &operator[](size_t index) const noexcept;

    T &operator[](size_t index) noexcept;

    void Swap(RawMemory &other) noexcept;

    const T *GetAddress() const noexcept;

    T *GetAddress() noexcept;

    size_t Capacity() const;

private:
    static T *Allocate(size_t n);

    static void Deallocate(T *buf) noexcept;

    T *buffer_ = nullptr;
    size_t capacity_ = 0;
};


template<typename T>
class Vector {
public:

    Vector() = default;

    explicit Vector(size_t size);

    Vector(const Vector &other);

    Vector(Vector &&other) noexcept: data_(std::move(other.data_)), size_(std::exchange(other.size_, 0)) {}

    ~Vector();

    void Reserve(size_t new_capacity);


    size_t Size() const noexcept;

    size_t Capacity() const noexcept;

    void Swap(Vector &other) noexcept {
        data_.Swap(other.data_), std::swap(size_, other.size_);
    }

    Vector &operator=(const Vector &other) {

        if (this != &other) {

            if (other.size_ <= data_.Capacity()) {

                if (size_ <= other.size_) {

                    std::copy(other.data_.GetAddress(),
                              other.data_.GetAddress() + size_,
                              data_.GetAddress());

                    std::uninitialized_copy_n(other.data_.GetAddress() + size_,
                                              other.size_ - size_,
                                              data_.GetAddress() + size_);
                } else {

                    std::copy(other.data_.GetAddress(),
                              other.data_.GetAddress() + other.size_,
                              data_.GetAddress());

                    std::destroy_n(data_.GetAddress() + other.size_,
                                   size_ - other.size_);
                }

                size_ = other.size_;

            } else {
                Vector other_copy(other);
                Swap(other_copy);
            }
        }

        return *this;
    }

    Vector &operator=(Vector &&other) noexcept {
        Swap(other);
        return *this;
    }

    const T &operator[](size_t index) const noexcept;

    T &operator[](size_t index) noexcept;

private:
    static T *Allocate(size_t n);

    static void Deallocate(T *buf) noexcept;

    static void DestroyN(T *buf, size_t n) noexcept;

    static void CopyConstruct(T *buf, const T &elem);

    static void Destroy(T *buf) noexcept;

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};

template<typename T>
void Vector<T>::Destroy(T *buf) noexcept {
    buf->~T();
}

template<typename T>
void Vector<T>::CopyConstruct(T *buf, const T &elem) {
    new(buf) T(elem);
}

template<typename T>
void Vector<T>::DestroyN(T *buf, size_t n) noexcept {
    for (size_t i = 0; i != n; ++i) {
        Destroy(buf + i);
    }
}

template<typename T>
void Vector<T>::Deallocate(T *buf) noexcept {

    operator delete(buf);
}

template<typename T>
T *Vector<T>::Allocate(size_t n) {
    return n != 0 ? static_cast<T *>(operator new(n * sizeof(T))) : nullptr;
}

template<typename T>
T &Vector<T>::operator[](size_t index) noexcept {

    assert(index < size_);
    return data_[index];
}

template<typename T>
const T &Vector<T>::operator[](size_t index) const noexcept {
    return const_cast<Vector &>(*this)[index];
}

template<typename T>
size_t Vector<T>::Capacity() const noexcept {
    return data_.Capacity();
}

template<typename T>
size_t Vector<T>::Size() const noexcept {
    return size_;
}

template<typename T>
void Vector<T>::Reserve(size_t new_capacity) {

    if (new_capacity <= data_.Capacity()) {
        return;
    }

    RawMemory<T> new_data(new_capacity);

    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
    } else {
        std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
    }

    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}


template<typename T>
Vector<T>::~Vector() {
    std::destroy_n(data_.GetAddress(), size_);
}

template<typename T>
Vector<T>::Vector(const Vector &other): data_(other.size_), size_(other.size_) {
    std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
}

template<typename T>
Vector<T>::Vector(size_t size)
        : data_(size), size_(size) {
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}


template<typename T>
void RawMemory<T>::Deallocate(T *buf) noexcept {
    operator delete(buf);
}

template<typename T>
T *RawMemory<T>::Allocate(size_t n) {
    return n != 0 ? static_cast<T *>(operator new(n * sizeof(T))) : nullptr;
}

template<typename T>
size_t RawMemory<T>::Capacity() const {
    return capacity_;
}

template<typename T>
T *RawMemory<T>::GetAddress() noexcept {
    return buffer_;
}

template<typename T>
const T *RawMemory<T>::GetAddress() const noexcept {
    return buffer_;
}

template<typename T>
void RawMemory<T>::Swap(RawMemory &other) noexcept {
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
}

template<typename T>
T &RawMemory<T>::operator[](size_t index) noexcept {
    assert(index < capacity_);
    return buffer_[index];
}

template<typename T>
const T &RawMemory<T>::operator[](size_t index) const noexcept {
    return const_cast<RawMemory &>(*this)[index];
}

template<typename T>
const T *RawMemory<T>::operator+(size_t offset) const noexcept {
    return const_cast<RawMemory &>(*this) + offset;
}

template<typename T>
T *RawMemory<T>::operator+(size_t offset) noexcept {
    assert(offset <= capacity_);
    return buffer_ + offset;
}

template<typename T>
RawMemory<T>::~RawMemory() {
    Deallocate(buffer_);
}

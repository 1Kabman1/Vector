#pragma once

#include <cassert>
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


template<typename T>
class Vector {
public:

    Vector() = default;

    explicit Vector(size_t size);

    Vector(const Vector &other);

    ~Vector();

    void Reserve(size_t new_capacity);


    size_t Size() const noexcept;

    size_t Capacity() const noexcept;

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
    size_t counter = 0;
    RawMemory<T> new_data(new_capacity);
    try {
        for (; counter != size_; ++counter) {
            CopyConstruct(new_data.GetAddress() + counter, data_[counter]);
        }
        DestroyN(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    catch (...) {
        DestroyN(new_data.GetAddress(), counter);
        throw;
    }
}


template<typename T>
Vector<T>::~Vector() {
    DestroyN(data_.GetAddress(), size_);
}

template<typename T>
Vector<T>::Vector(const Vector &other)
        : data_(other.size_), size_(other.size_) {
    size_t counter = 0;
    try {
        for (; counter != other.size_; ++counter) {
            CopyConstruct(data_.GetAddress() + counter, other.data_[counter]);
        }
    }
    catch (...) {
        DestroyN(data_.GetAddress(), counter);
        throw;
    }
}

template<typename T>
Vector<T>::Vector(size_t size)
        : data_(size), size_(size) {
    size_t counter = 0;
    try {
        for (; counter != size; ++counter) {
            new(data_ + counter) T();
        }
    }
    catch (...) {
        DestroyN(data_.GetAddress(), counter);
        throw;
    }
}

#pragma once

#include <gsl/gsl-lite.hpp>

#include <cassert>

namespace spacemma
{
    template <typename T>
    class Buffer final
    {
    public:
        Buffer(size_t size);
        ~Buffer();
        Buffer(Buffer&) = delete;
        Buffer(Buffer&&) = delete;
        Buffer& operator=(Buffer&) = delete;
        Buffer& operator=(Buffer&&) = delete;
        T* getPointer() const;
        size_t getTotalSize() const;
        size_t getTotalByteSize() const;
        size_t getUsedSize() const;
        size_t getUsedByteSize() const;
        void setUsedSize(size_t usedSize);
        void setUsedByteSize(size_t usedByteSize);
        gsl::span<T> getSpan() const;
        template <typename U>
        gsl::span<U> getSpan() const;
    private:
        T* data;
        size_t size, byteSize, usedSize;
    };

    template <typename T>
    Buffer<T>::Buffer(size_t size) : data(new T[size]), size(size), byteSize(size * sizeof(T)), usedSize(size) {}

    template <typename T>
    Buffer<T>::~Buffer()
    {
        delete[] data;
        data = nullptr;
        size = 0UL;
    }

    template <typename T>
    T* Buffer<T>::getPointer() const
    {
        return data;
    }

    template <typename T>
    size_t Buffer<T>::getTotalSize() const
    {
        return size;
    }

    template <typename T>
    size_t Buffer<T>::getTotalByteSize() const
    {
        return byteSize;
    }

    template <typename T>
    size_t Buffer<T>::getUsedSize() const
    {
        return usedSize;
    }

    template <typename T>
    size_t Buffer<T>::getUsedByteSize() const
    {
        return usedSize * sizeof(T);
    }

    template <typename T>
    void Buffer<T>::setUsedSize(size_t usedSize)
    {
        assert(usedSize >= 0ULL && usedSize <= size);
        this->usedSize = usedSize;
    }

    template <typename T>
    void Buffer<T>::setUsedByteSize(size_t usedByteSize)
    {
        assert(usedByteSize % sizeof(T) == 0);
        assert(usedSize >= 0ULL && usedSize <= byteSize);
        this->usedSize = usedByteSize / sizeof(T);
    }

    template <typename T>
    template <typename U>
    gsl::span<U> Buffer<T>::getSpan() const
    {
        assert((usedSize * sizeof(T)) % sizeof(U) == 0);
        return gsl::make_span<U>(reinterpret_cast<U*>(data), usedSize * sizeof(T) / sizeof(U));
    }

    using ByteBuffer = Buffer<uint8_t>;
}

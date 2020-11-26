#pragma once

#include <gsl/gsl-lite.hpp>

#include <cassert>

namespace spacemma
{
    /**
     * A template class representing a data buffer.
     */
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
        /**
         * Returns the raw pointer to the data.
         */
        T* getPointer() const;
        /**
         * Returns the total size of the underlying buffer.
         */
        size_t getTotalSize() const;
        /**
         * Returns the total byte size of the underlying buffer.
         */
        size_t getTotalByteSize() const;
        /**
         * Returns the size of the data that is currently used.
         */
        size_t getUsedSize() const;
        /**
         * Returns the byte size of the data that is currently used.
         */
        size_t getUsedByteSize() const;
        /**
         * Sets the buffer's used size. The provided size cannot be greater than the total size of this buffer.
         */
        void setUsedSize(size_t usedSize);
        /**
         * Sets the buffer's used byte size. The provided size cannot be greater than the total byte size of this buffer.
         */
        void setUsedByteSize(size_t usedByteSize);
        /**
         * Returns a span representation of this buffer.
         */
        gsl::span<T> getSpan() const;
        /**
         * Returns a span representation of this buffer casted to type provided by a template argument.
         */
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

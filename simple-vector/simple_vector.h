#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity) : capacity_(capacity) {}

    size_t capacity_{};
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size) : SimpleVector(size, Type()) {}

    SimpleVector(size_t size, const Type& value) : items_{size}, size_{size}, capacity_{size} {
        std::fill(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init) : size_{init.size()}, capacity_{init.size()}  {
        MakeNewCopyVector(init.begin(), init.end());        
    }

    SimpleVector(const SimpleVector& other) : size_{other.size_}, capacity_{other.size_}  {
        MakeNewCopyVector(other.begin(), other.end());
    }

    SimpleVector(SimpleVector&& other) {
        MakeNewMoveVector(other.size_, other.begin(), other.end());
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }

    SimpleVector(ReserveProxyObj a) {
        ArrayPtr<Type> tmp(a.capacity_);
        items_.swap(tmp);
        capacity_ = a.capacity_;
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (&rhs == this) {
            return *this;
        }
        SimpleVector tmp{rhs};
        swap(tmp);
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (&rhs == this) {
            return *this;
        }
        items_.swap(rhs.items_);                 
        size_ = std::exchange(rhs.size_, 0);
        capacity_ = std::exchange(rhs.capacity_, 0);
        delete [] rhs.items_.Release();  
        return *this;
    }

    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            size_t new_capacity{(capacity_) ? (capacity_ * 2) : 1};
            MakeNewMoveVector(new_capacity, begin(), end());
            capacity_ = new_capacity;
        }
        items_[size_] = item;
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            size_t new_capacity{(capacity_) ? (capacity_ * 2) : 1};
            MakeNewMoveVector(new_capacity, begin(), end());
            capacity_ = new_capacity;
        }
        items_[size_] = std::move(item);
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        size_t index{static_cast<size_t>(std::distance(cbegin(), pos))};
        if (size_ == capacity_) {
            size_t new_capacity{capacity_ ? (capacity_ * 2) : 1};
            MakeNewMoveVector(new_capacity, begin(), end());
            capacity_ = new_capacity;
        }
        items_[index] = value;
        ++size_;
        return begin() + index;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        size_t index{static_cast<size_t>(std::distance(cbegin(), pos))};
        if (size_ == capacity_) {
            size_t new_capacity{capacity_ ? (capacity_ * 2) : 1};
            MakeNewMoveVector(new_capacity, begin(), end());
            capacity_ = new_capacity;
        }
        items_[index] = std::move(value);
        ++size_;
        return begin() + index;
    }

    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        assert(!IsEmpty());
        size_t index{static_cast<size_t>(std::distance(cbegin(), pos))};
        std::move(begin() + index + 1, end(), begin() + index);
        --size_;
        return begin() + index;
    }

    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            MakeNewMoveVector(new_capacity, begin(), end());
            capacity_ = new_capacity;
        }
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return (size_ == 0);
    }

    Type& operator[](size_t index) noexcept {
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return items_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("bad");
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("bad");
        }
        return items_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
            return;
        }
        if (new_size <= capacity_) {
            auto it = end();
            while (it != begin() + new_size) {
                *it = Type();
                ++it;
            }
            size_ = new_size;
            return;
        }
        size_t new_capacity{std::max(new_size, capacity_ * 2)};
        MakeNewMoveVector(new_capacity, begin(), end());
        auto it = end();
        while (it != begin() + new_size) {
            *it = Type(); 
            ++it;
        }
        size_ = new_size;
        capacity_ = new_capacity;
    }

    Iterator begin() noexcept {
        return items_.Get();
    }

    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }



private:
    ArrayPtr<Type> items_;

    size_t size_ = 0;
    size_t capacity_ = 0;

    template <typename It>
    void MakeNewCopyVector(It begin, It end) {
        ArrayPtr<Type> tmp(static_cast<size_t>(std::distance(begin, end)));
        std::copy(begin, end, tmp.Get());
        items_.swap(tmp);
    }

    // Этот метод используется в методах, где новый размер вектора не совпадает с расстоянием от begin до end: Resize, Insert, PushBack, Reserve
    // Исключение состовляет только конструктор SimpleVector(SimpleVector&& other)
    // Поэтому в этом методе параметр new_value оставил
    template <typename It>
    void MakeNewMoveVector(size_t new_value, It begin, It end) {
        ArrayPtr<Type> tmp(new_value);
        std::move(begin, end, tmp.Get());
        items_.swap(tmp);
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}

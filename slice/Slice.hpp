#include <span>
#include <concepts>
#include <cstdlib>
#include <array>
#include <iterator>
#include <vector>
#include <lib/assert.hpp>
inline constexpr std::ptrdiff_t dynamic_stride = -1;

namespace util {
  static constexpr inline size_t DivWithRounding(size_t a, size_t b) {
    return (a + b - 1) / b;
  }
}

namespace base {
  template <std::size_t extent>
  class SliceExtentBase {
   public:
    constexpr SliceExtentBase() noexcept = default;
    SliceExtentBase(std::size_t) {
    }

    static constexpr std::size_t Extent() {
      return extent;
    }
  };

  template <>
  class SliceExtentBase<std::dynamic_extent> {
   public:
    constexpr SliceExtentBase() noexcept = default;
    SliceExtentBase(std::size_t extent): extent_(extent) {}
    std::size_t Extent() const {
      return extent_;
    }

   private:
    std::size_t extent_;
  };

  template <std::ptrdiff_t stride>
  class SliceStrideBase {
   public:
    constexpr SliceStrideBase() noexcept = default;
    SliceStrideBase(std::ptrdiff_t) {}
    static constexpr std::ptrdiff_t Stride() {
      return stride;
    }
  };

  template <>
  class SliceStrideBase<dynamic_stride> {
   public:
    constexpr SliceStrideBase() noexcept = default;
    SliceStrideBase(std::ptrdiff_t stride): stride_(stride) {}
    std::ptrdiff_t Stride() const {
      return stride_;
    }
   private:
    std::ptrdiff_t stride_;
  };
}

template <class T, std::size_t extent = std::dynamic_extent, std::ptrdiff_t stride = 1u>
class Slice: public base::SliceExtentBase<extent>, public base::SliceStrideBase<stride> {
 public:
  class Iterator {
   public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_cv_t<T>;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::contiguous_iterator_tag;
    using size_type = std::size_t;

    Iterator() = default;
    Iterator(T* data, std::ptrdiff_t stride_arg): data_(data), stride_(stride_arg) {}

    reference operator*() const {
      return *data_;
    }
    pointer operator->() {
      return data_;
    }
    reference operator[](size_type idx) const {
      return data_[idx];
    }

    Iterator& operator++() {
      data_ += stride_;
      return *this;
    }
    Iterator& operator--() {
      data_ -= stride_;
      return *this;
    }
    Iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }
    Iterator operator--(int) {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    Iterator& operator+=(size_type plus) {
      data_ += stride_ * plus;
      return *this;
    }
    Iterator& operator-=(size_type minus) {
      data_ -= stride_ * minus;
      return *this;
    }
    Iterator operator+(size_type plus) const {
      Iterator tmp = *this;
      tmp += plus;
      return tmp;
    }
    Iterator operator-(size_type minus) const {
      Iterator tmp = *this;
      tmp -= minus;
      return tmp;
    }

    difference_type operator-(const Iterator& other) const {
      return (data_ - other.data_) / stride_;
    }
    friend Iterator operator+(size_type left, const Iterator& right) {
      return right + left;
    }

    inline bool operator==(const Iterator& other) const {
      return data_ == other.data_;
    }
    inline bool operator!=(const Iterator& other) const {
      return data_ != other.data_;
    }
    inline bool operator>(const Iterator& other) const {
      return data_ > other.data_;
    }
    inline bool operator<(const Iterator& other) const {
      return data_ < other.data_;
    }
    inline bool operator>=(const Iterator& other) const {
      return data_ >= other.data_;
    }
    inline bool operator<=(const Iterator& other) const {
      return data_ <= other.data_;
    }

   private:
    T* data_;
    std::ptrdiff_t stride_;
  };

  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = Iterator;

  constexpr Slice() noexcept = default;
  Slice(const Slice&) = default;
  explicit Slice(Slice&&) = default;
  Slice& operator=(const Slice&) = default;
  Slice& operator=(Slice&&) = default;

  template <class It>
  requires std::contiguous_iterator<It>
  Slice(It begin, size_type count, difference_type skip = 1u)
    : base::SliceExtentBase<extent>(util::DivWithRounding(count, skip)),
      base::SliceStrideBase<stride>(skip), data_(&(*begin)) {
    MPC_VERIFY(extent == std::dynamic_extent || extent <= util::DivWithRounding(count, skip));
  }

  template <class V>
  Slice(std::vector<V>& container)
    : Slice(container.data(), container.size(), stride) {
  }

  template <class V>
  Slice(std::vector<V>& container, difference_type skip)
    : Slice(container.data(), container.size(), stride) {
    MPC_VERIFY(stride == skip);
  }

  template <class V, size_type sz>
  Slice(std::array<V, sz>& container)
    : Slice(container.data(), container.size(), stride) {
  }

  template <class U, size_type otherExtent, difference_type otherStride>
  operator Slice<U, otherExtent, otherStride>() const {
    return Slice<U, otherExtent, otherStride>(data_, this->Size() * this->Stride(), this->Stride());
  }

  iterator begin() const {
    return Iterator(data_, this->Stride());
  }

  iterator end() const {
    return Iterator(data_ + this->Size() * this->Stride(), this->Stride());
  }

  auto rbegin() const {
    return std::reverse_iterator(end());
  }

  auto rend() const {
    return std::reverse_iterator(begin());
  }

  reference operator[](size_type idx) const {
    MPC_VERIFY(idx < this->Size());
    return data_[idx * this->Stride()];
  }

  T* Data() const {
    return data_;
  }

  constexpr size_type Size() const noexcept {
    return this->Extent();
  }

  template<class U, size_type other_extent, difference_type other_stride>
  bool operator==(const Slice<U, other_extent, other_stride>& other_slice) const {
    auto other_it = other_slice.begin();
    for (auto it = begin(); it != end(); ++it, ++other_it) {
      if (other_it == other_slice.end()) {
        return false;
      }
      if (*it != *other_it) {
        return false;
      }
    }
    if (other_it != other_slice.end()) {
      return false;
    }
    return true;
  }

  Slice<T, std::dynamic_extent, stride>
  First(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_, count * this->Stride(), this->Stride());
  }

  template <std::size_t count>
  Slice<T, count, stride>
  First() const {
    return Slice<T, count, stride>(data_, count * this->Stride(), this->Stride());
  }

  Slice<T, std::dynamic_extent, stride>
  Last(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_ + this->Size() * this->Stride() - count * this->Stride(), count * this->Stride(), this->Stride());
  }

  template <std::size_t count>
  Slice<T, count, stride>
  Last() const {
    return Slice<T, count, stride>(data_ + this->Size() * this->Stride() - count * this->Stride(), count * this->Stride(), this->Stride());
  }

 private:
  static constexpr size_type DecreasedExtent(const difference_type drop) {
    if constexpr (extent == std::dynamic_extent) {
      return std::dynamic_extent;
    }
    return extent - drop;
  }

 public:
  Slice<T, std::dynamic_extent, stride>
  DropFirst(std::size_t count) const {
    MPC_VERIFY(count <= this->Size());
    return Slice<T, std::dynamic_extent, stride>(data_ + count * stride, (this->Size() - count) * this->Stride(), this->Stride());
  }

  template <std::size_t count>
  Slice<T, DecreasedExtent(count), stride>
  DropFirst() const {
    MPC_VERIFY(count <= this->Size());
    return Slice<T, DecreasedExtent(count), stride>(data_ + count * stride, (this->Size() - count) * this->Stride(), this->Stride());
  }

  Slice<T, std::dynamic_extent, stride>
  DropLast(std::size_t count) const {
    MPC_VERIFY(count <= this->Size());
    return Slice<T, std::dynamic_extent, stride>(data_, (this->Size() - count) * this->Stride(), this->Stride());
  }

  template <std::size_t count>
  Slice<T, DecreasedExtent(count), stride>
  DropLast() const {
    MPC_VERIFY(count <= this->Size());
    return Slice<T, DecreasedExtent(count), stride>(data_, (this->Size() - count) * this->Stride(), this->Stride());
  }

 private:
  static constexpr size_type SkippedExtent(const difference_type skip) {
    if constexpr (extent == std::dynamic_extent) {
      return std::dynamic_extent;
    }
    return util::DivWithRounding(extent, skip);
  }

  static constexpr difference_type SkippedStride(const difference_type skip) {
    if constexpr (stride == dynamic_stride) {
      return dynamic_stride;
    }
    return stride * skip;
  }

 public:
  Slice<T, std::dynamic_extent, dynamic_stride>
  Skip(std::ptrdiff_t skip) const {
    return Slice<T, std::dynamic_extent, dynamic_stride>(data_, this->Size() * this->Stride(), this->Stride() * skip);
  }

  template <difference_type skip>
  Slice<T, SkippedExtent(skip), SkippedStride(skip)> Skip() const {
    return Slice<T, SkippedExtent(skip), SkippedStride(skip)>(data_, this->Size() * this->Stride(), this->Stride() * skip);
  }
 private:
  T* data_;
};

template<class T>
Slice(const std::vector<T>& container) -> Slice<T>;

template<class T, std::size_t Size>
Slice(const std::array<T, Size>& container) -> Slice<T, Size>;

template<class It>
Slice(It begin, std::size_t count, std::ptrdiff_t skip) -> Slice<typename std::iterator_traits<It>::value_type, std::dynamic_extent, dynamic_stride>;


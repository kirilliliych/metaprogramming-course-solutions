#include <span>
#include <concepts>
#include <cstdlib>
#include <iterator>


template <std::size_t extent>
struct SpanBase {
  constexpr SpanBase() noexcept = default;
  SpanBase(std::size_t extent_arg) {
    MPC_VERIFY(extent == extent_arg);
  }

  constexpr std::size_t Extent() const {
    return extent;
  }
};

template <>
struct SpanBase<std::dynamic_extent> {
  constexpr SpanBase() noexcept = default;
  SpanBase(std::size_t extent) : extent_(extent) {}

  std::size_t Extent() const {
    return extent_;
  }

 private:
  std::size_t extent_ = 0;
};


template <class T, std::size_t extent = std::dynamic_extent>
class Span : public SpanBase<extent> {
  // Reimplement the standard span interface here
  // (some more exotic methods are not checked by the tests and can be sipped)
  // Note that unliike std, the methods name should be Capitalized!
  // E.g. instead of subspan, do Subspan.
  // Note that this does not apply to iterator methods like begin/end/etc.
 public:
  using SpanBase<extent>::Extent;

  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type =	std::size_t;
  using difference_type	= std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr Span() noexcept requires (extent == 0 || extent == std::dynamic_extent) = default;

  template <std::contiguous_iterator It>
  explicit(extent != std::dynamic_extent)
  constexpr Span(It first, size_type count) : SpanBase<extent>(count), data_(std::to_address(first)) {
  }

  template <std::contiguous_iterator It, std::contiguous_iterator End>
  requires(std::is_convertible_v<End, std::size_t> && std::same_as<typename It::value_type, typename End::value_type>)
  explicit(extent != std::dynamic_extent)
  constexpr Span(It first, End last) : SpanBase<extent>(last - first), data_(std::to_address(first)) {}

  template <std::size_t N>
  requires(extent == std::dynamic_extent || N == extent)
  constexpr Span(std::type_identity_t<element_type> (&arr)[N]) noexcept : SpanBase<extent>(N), data_(std::data(arr)) {}

  template <class U, std::size_t N>
  requires(extent == std::dynamic_extent || N == extent)
  constexpr Span(std::array<U, N>& arr) noexcept : SpanBase<extent>(N), data_(std::data(arr)) {}

  template <class U, std::size_t N>
  requires(extent == std::dynamic_extent || N == extent)
  constexpr Span(const std::array<U, N>& arr) noexcept : SpanBase<extent>(N), data_(std::data(arr)) {}

  template <class R>
  requires(!std::is_array_v<std::remove_cvref_t<R>>)
  explicit(extent != std::dynamic_extent)
  constexpr Span(R&& range) : SpanBase<extent>(range.size()), data_(std::data(range)) {}

  explicit(extent != std::dynamic_extent)
  constexpr Span(std::initializer_list<value_type> il) noexcept
    : Span(il.begin(), il.size()) {
      MPC_VERIFY(extent == std::dynamic_extent || il.size() == extent);
    }  

  template <class U, std::size_t N>
  requires(extent == std::dynamic_extent || N == std::dynamic_extent || N == extent)
  explicit(extent != std::dynamic_extent && N == std::dynamic_extent)
  constexpr Span(const Span<U, N>& source) noexcept : SpanBase<extent>(source.size()), data_(source.data()) {}

  constexpr Span(const Span& other) noexcept = default;

  ~Span() noexcept = default;

  Span& operator=(const Span& other) noexcept = default;

  constexpr iterator begin() const noexcept { 
    return Data(); 
  }

  constexpr const_iterator cbegin() const noexcept {
    return const_pointer(begin());
  }

  constexpr iterator end() const noexcept { 
    return Data() + Size(); 
  }

  constexpr const_iterator cend() const noexcept {
    return const_pointer(end());
  }

  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }

  constexpr const_reverse_iterator crbegin() const noexcept {
    return reverse_iterator(cend());
  }

  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }

  constexpr const_reverse_iterator crend() const noexcept {
    return reverse_iterator(cbegin());
  }

  constexpr reference Front() const {
    MPC_VERIFY(Size() > 0);
    return *Data();
  }

  constexpr reference Back() const {
    MPC_VERIFY(Size() > 0);
    return *(Data() + (Size() - 1));
  }

  constexpr reference At(size_type pos) const {
    if (pos >= Size()) {
      throw std::out_of_range("At() call argument is out of bounds");
    }
    return operator[](pos);
  }

  constexpr reference operator[](size_type idx) const {
    MPC_VERIFY(idx < Size());
    return *(Data() + idx);
  }

  constexpr pointer Data() const noexcept {
    return data_;
  }

  constexpr size_type Size() const noexcept {
    return Extent();
  }

  constexpr size_type SizeBytes() const noexcept {
    return Size() * sizeof(element_type);
  }

  constexpr bool Empty() const noexcept {
    return Size() == 0;
  }

  template <std::size_t Count>
  constexpr Span<element_type, Count> First() const {
    MPC_VERIFY(Count < Size());
    return Span<element_type, Count>(Data(), Count);
  }

  constexpr Span<element_type, std::dynamic_extent> First(size_type Count) const {
    MPC_VERIFY(Count < Size());
    return Span<element_type, std::dynamic_extent>(Data(), Count);
  }

  template <std::size_t Count>
  constexpr Span<element_type, Count> Last() const {
    MPC_VERIFY(Count < Size());
    return Span<element_type, Count>(Data() + (Size() - Count), Count);
  }

  constexpr Span<element_type, std::dynamic_extent> Last(size_type Count) const {
    MPC_VERIFY(Count < Size());
    return Span<element_type, std::dynamic_extent>(Data() + (Size() - Count), Count);
  }

 private:
  template <std::size_t Offset, std::size_t Count>
  static constexpr std::size_t SubspanDeduceExtent_() {
    if constexpr (Count != std::dynamic_extent) {
        return Count;
    } else if constexpr (extent != std::dynamic_extent) {
        return extent - Offset;
    }
    return std::dynamic_extent;
  }

 public:
  template <size_t Offset, size_t Count = std::dynamic_extent>
  constexpr Span<element_type, SubspanDeduceExtent_<Offset, Count>> Subspan() const {
    if constexpr (Count == std::dynamic_extent) {
      return Span<element_type, Size() - Count>(Data() + Offset, Size() - Offset);
    }
    return Span<element_type, Count>(Data() + Offset, Count);
  }

  constexpr Span<element_type, std::dynamic_extent> Subspan(size_type Offset, size_type Count = std::dynamic_extent) const {
    if (Count == std::dynamic_extent) {
      return Span<element_type, std::dynamic_extent>(Data() + Offset, Size() - Offset);
    }
    return Span<element_type, std::dynamic_extent>(Data() + Offset, Count);
  }

 private:
  T* data_ = nullptr;
};

template <typename T, size_t N>
Span<const std::byte, N == std::dynamic_extent ? std::dynamic_extent : N * sizeof(T)> AsBytes(Span<T, N> s) noexcept {
  return (reinterpret_cast<const std::byte*>(s.data(), s.size_bytes()), s.size_bytes());
}

template <typename T, size_t N>
Span<std::byte, N == std::dynamic_extent ? std::dynamic_extent : N * sizeof(T)> AsWritableBytes(Span<T, N> s) noexcept {
  return (reinterpret_cast<std::byte*>(s.data(), s.size_bytes()), s.size_bytes());
}


template <class It, class EndOrSize>
Span(It, EndOrSize) -> Span<std::remove_reference_t<std::iter_reference_t<It>>>;

template<class T, std::size_t N>
Span(T (&)[N]) -> Span<T, N>;

template <class T, std::size_t N>
Span(std::array<T, N>&) -> Span<T, N>;

template <class T, std::size_t N>
Span(const std::array<T, N>&) -> Span<const T, N>;

template <class R>
Span(R&&) -> Span<std::remove_reference_t<std::ranges::range_reference_t<R>>>;

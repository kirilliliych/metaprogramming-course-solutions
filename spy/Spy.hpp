#pragma once
#include <array>
#include <memory>
#include <utility>

constexpr std::size_t SBO_SIZE = 64;

struct LogMeta {
  unsigned int accessCount = 0;
  unsigned int unprocessedCallsCount = 0;
};

template <class T, class Allocator = std::allocator<std::byte>>
class Spy {
  template <class U, class Alloc>
  class SpyHelper {
   public:
    SpyHelper(Spy<T, Allocator>* spy) : spy_(spy) {}
    
    T* operator->() {
      ++(*spy_).logInfo_.accessCount;
      ++(*spy_).logInfo_.unprocessedCallsCount;
      return &((*spy_).value_);
    }

    ~SpyHelper() {
      if (--(*spy_).logInfo_.unprocessedCallsCount == 0) {
        (*spy_).call_((*spy_).logger_, (*spy_).logInfo_.accessCount);
        (*spy_).logInfo_.accessCount = 0;
      }
    }

   private:
    Spy<T, Allocator>* spy_;
  };

 public:
  using AllocTraits = std::allocator_traits<Allocator>;

  Spy() = default;

  Spy(T& value, const Allocator& alloc = Allocator()) noexcept
    requires std::copyable<T>
    : value_(value), allocator_(alloc) {}

  Spy(T&& value, const Allocator& alloc = Allocator()) noexcept
    requires std::movable<T>
    : value_(std::move(value)), allocator_(alloc) {}

  explicit Spy(const Allocator& alloc) noexcept
    : allocator_(alloc) {}

  Spy(const Spy& other)
    requires std::copyable<T>
    : value_(other.value_) {

    if (other.copy_) {
      other.copy_(logger_, other.logger_, other.allocator_);
    }
    call_ = other.call_;
    copy_ = other.copy_;
    move_ = other.move_;
    destructor_ = other.destructor_;
  }

  Spy& operator=(const Spy& other)
    requires std::copyable<T> {
    if (this != &other) {
      resetLogger();

      destructor_ = other.destructor_;
      call_ = other.call_; 
      copy_ = other.copy_;
      move_ = other.move_;
      value_ = other.value_;
    
      if (copy_) {
        copy_(logger_, other.logger_, allocator_);
      }

      if constexpr (std::is_same_v<typename AllocTraits::propagate_on_container_copy_assignment, std::true_type>)
        allocator_ = other.allocator_;
      
      logInfo_.accessCount = 0;
      logInfo_.unprocessedCallsCount = 0;
    }
    return *this;
  }
  
  Spy(Spy&& other) noexcept
    requires std::movable<T>
    : value_(std::move(other.value_)) {
    
    if (other.move_) {
      other.move_(logger_, other.logger_, other.allocator_);
    }
    call_ = std::exchange(other.call_, nullptr);
    copy_ = std::exchange(other.copy_, nullptr);
    move_ = std::exchange(other.move_, nullptr);
    destructor_ = std::exchange(other.destructor_, nullptr);
    allocator_ = other.allocator_;
  }

  Spy& operator=(Spy&& other)
    requires std::movable<T> {
    if (this != &other) {
      resetLogger();
      
      call_ = std::exchange(other.call_, nullptr);
      copy_ = std::exchange(other.copy_, nullptr);
      move_ = std::exchange(other.move_, nullptr);
      destructor_ = other.destructor_;

      if constexpr (!std::is_same_v<typename AllocTraits::propagate_on_container_move_assignment, std::true_type>) {
        if (allocator_ != other.allocator_ && copy_) {
          copy_(logger_, other.logger_, allocator_);
          allocator_ = other.allocator_;
          other.resetLogger();
        } else if (move_) {
          move_(logger_, other.logger_, other.allocator_);
          other.destructor_ = nullptr;
        }
      } else if (move_) {
          move_(logger_, other.logger_, other.allocator_);
          other.destructor_ = nullptr;
      }
    
      value_ = std::move(other.value_);
      logInfo_.accessCount = other.logInfo_.accessCount;
      logInfo_.unprocessedCallsCount = other.logInfo_.unprocessedCallsCount;
      other.logInfo_ = LogMeta();
    }
    return *this;
  }

  ~Spy() {
    resetLogger();
  }

  T& operator*() {
    return value_;
  }

  const T& operator*() const {
    return value_;
  }

  SpyHelper<T, Allocator> operator->() {
    return SpyHelper<T, Allocator>(this);
  }

  constexpr bool operator==(const Spy& other) const
    requires std::equality_comparable<T> {
    return value_ == other.value_;
  }

  template <std::invocable<unsigned int> Logger>
    requires ((!std::copyable<T> || std::copy_constructible<Logger>) &&
              (!std::movable<T> || std::move_constructible<Logger>))
  void setLogger(Logger&& logger) {
    using LoggerNoRef = std::remove_reference_t<Logger>;
    resetLogger();
    if constexpr (sizeof(Logger) <= SBO_SIZE) {
      AllocTraits::construct(allocator_, reinterpret_cast<LoggerNoRef*>(logger_.sboLogger.data()), std::forward<Logger>(logger));
    } else {
      logger_.unlimitedLogger = allocator_.allocate(sizeof(Logger));
      AllocTraits::construct(allocator_, reinterpret_cast<LoggerNoRef*>(logger_.unlimitedLogger), std::forward<Logger>(logger));
    }
    call_ = &call<LoggerNoRef>;
    copy_ = &copy<LoggerNoRef>;
    move_ = &move<LoggerNoRef>;
    destructor_ = &destroy<LoggerNoRef>;
  }

 private:
  union LoggerType {
    void* unlimitedLogger;
    std::array<std::byte, SBO_SIZE> sboLogger;
  };

  template<class LoggerImpl>
  static void call(LoggerType& ptr, unsigned int arg) {
    if constexpr (sizeof(LoggerImpl) <= SBO_SIZE) {
      LoggerImpl* logger = reinterpret_cast<LoggerImpl*>(ptr.sboLogger.data());
      (*logger)(arg);
    } else {
      (*static_cast<LoggerImpl*>(ptr.unlimitedLogger))(arg);
    }
  }

  template<class LoggerImpl>
  static void copy(LoggerType& to, const LoggerType& from, Allocator allocator) {
    if constexpr (std::copy_constructible<LoggerImpl>) {
      if constexpr (sizeof(LoggerImpl) <= SBO_SIZE) {
        AllocTraits::construct(allocator, reinterpret_cast<LoggerImpl*>(to.sboLogger.data()), *(reinterpret_cast<const LoggerImpl*>(from.sboLogger.data())));
      } else {
        to.unlimitedLogger = allocator.allocate(sizeof(LoggerImpl));
        AllocTraits::construct(allocator, static_cast<LoggerImpl*>(to.unlimitedLogger), *static_cast<LoggerImpl*>(from.unlimitedLogger));
      }
    }
  }

  template<class LoggerImpl>
  static void move(LoggerType& to, LoggerType& from, Allocator allocator) {
    if constexpr (sizeof(LoggerImpl) <= SBO_SIZE) {
      auto* logger = const_cast<LoggerImpl*>(reinterpret_cast<const LoggerImpl*>(from.sboLogger.data()));
      AllocTraits::construct(allocator, reinterpret_cast<LoggerImpl*>(to.sboLogger.data()), std::move(*logger));
      AllocTraits::destroy(allocator, logger);
    } else {
      to.unlimitedLogger = std::exchange(from.unlimitedLogger, nullptr);
    }
  }

  template <class LoggerImpl>
  static void destroy(LoggerType& ptr, Allocator allocator) {
    if constexpr (sizeof(LoggerImpl) <= SBO_SIZE) {
      AllocTraits::destroy(allocator, reinterpret_cast<LoggerImpl*>(ptr.sboLogger.data()));
    } else {
      AllocTraits::destroy(allocator, static_cast<LoggerImpl*>(ptr.unlimitedLogger));
      allocator.deallocate(static_cast<std::byte*>(ptr.unlimitedLogger), sizeof(LoggerImpl));
    }
  }

  void resetLogger() {
    if (destructor_ != nullptr) {
      destructor_(logger_, allocator_);
      call_ = nullptr;
      copy_ = nullptr;
      move_ = nullptr;
      destructor_ = nullptr;
    }
  }

  void(*call_)(LoggerType&, unsigned int) = nullptr;
  void(*copy_)(LoggerType&, const LoggerType&, Allocator) = nullptr;
  void(*move_)(LoggerType&, LoggerType&, Allocator) = nullptr;
  void(*destructor_)(LoggerType&, Allocator) = nullptr;
  
  T value_;
  Allocator allocator_;

  LoggerType logger_{nullptr};
 public:
  LogMeta logInfo_;
};

#pragma once

#include <stdexcept>
#include <variant>

namespace astra::outcome {

/// Unit type - represents "no value" for Result<void, E> equivalent
struct Unit {
  constexpr bool operator==(const Unit &) const noexcept {
    return true;
  }
  constexpr bool operator!=(const Unit &) const noexcept {
    return false;
  }
};

/**
 * @brief Result<T, E> for error handling without exceptions
 *
 * Industry standard: Boost.Outcome, Google StatusOr, Rust Result
 * Use for expected errors (network, DB, validation)
 * NOT for programmer errors (use exceptions/assertions)
 */
template <typename T, typename E> class Result {
public:
  static Result Ok(T value) {
    return Result(std::move(value));
  }

  static Result Err(E error) {
    return Result(std::move(error));
  }

  [[nodiscard]] bool is_ok() const noexcept {
    return std::holds_alternative<T>(m_data);
  }

  [[nodiscard]] bool is_err() const noexcept {
    return std::holds_alternative<E>(m_data);
  }

  [[nodiscard]] T &value() & {
    if (is_err()) {
      throw std::logic_error("Attempted to get value from error Result");
    }
    return std::get<T>(m_data);
  }

  [[nodiscard]] const T &value() const & {
    if (is_err()) {
      throw std::logic_error("Attempted to get value from error Result");
    }
    return std::get<T>(m_data);
  }

  [[nodiscard]] T &&value() && {
    if (is_err()) {
      throw std::logic_error("Attempted to get value from error Result");
    }
    return std::get<T>(std::move(m_data));
  }

  [[nodiscard]] E &error() & {
    if (is_ok()) {
      throw std::logic_error("Attempted to get error from ok Result");
    }
    return std::get<E>(m_data);
  }

  [[nodiscard]] const E &error() const & {
    if (is_ok()) {
      throw std::logic_error("Attempted to get error from ok Result");
    }
    return std::get<E>(m_data);
  }

  [[nodiscard]] T value_or(T default_value) const & {
    return is_ok() ? std::get<T>(m_data) : std::move(default_value);
  }

  explicit operator bool() const noexcept {
    return is_ok();
  }

private:
  explicit Result(T value) : m_data(std::move(value)) {
  }
  explicit Result(E error) : m_data(std::move(error)) {
  }

  std::variant<T, E> m_data;
};

/**
 * @brief Partial specialization for Result<void, E>
 *
 * Uses Unit internally but provides void-like interface
 */
template <typename E> class Result<void, E> {
public:
  static Result Ok() {
    return Result(Unit{});
  }

  static Result Err(E error) {
    return Result(std::move(error));
  }

  [[nodiscard]] bool is_ok() const noexcept {
    return std::holds_alternative<Unit>(m_data);
  }

  [[nodiscard]] bool is_err() const noexcept {
    return std::holds_alternative<E>(m_data);
  }

  [[nodiscard]] E &error() & {
    if (is_ok()) {
      throw std::logic_error("Attempted to get error from ok Result");
    }
    return std::get<E>(m_data);
  }

  [[nodiscard]] const E &error() const & {
    if (is_ok()) {
      throw std::logic_error("Attempted to get error from ok Result");
    }
    return std::get<E>(m_data);
  }

  explicit operator bool() const noexcept {
    return is_ok();
  }

private:
  explicit Result(Unit) : m_data(Unit{}) {
  }
  explicit Result(E error) : m_data(std::move(error)) {
  }

  std::variant<Unit, E> m_data;
};

} // namespace astra::outcome

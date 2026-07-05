#pragma once

#include <stdexcept>
#include <utility>
#include <variant>

namespace qscd::core {

template <class T, class E>
class Result {
public:
  static Result success(T value) { return Result(std::move(value)); }
  static Result failure(E error) { return Result(std::move(error)); }

  bool hasValue() const { return std::holds_alternative<T>(data_); }
  explicit operator bool() const { return hasValue(); }

  const T& value() const { return std::get<T>(data_); }
  T& value() { return std::get<T>(data_); }

  const E& error() const { return std::get<E>(data_); }
  E& error() { return std::get<E>(data_); }

private:
  explicit Result(T value) : data_(std::move(value)) {}
  explicit Result(E error) : data_(std::move(error)) {}

  std::variant<T, E> data_;
};

} // namespace qscd::core

#ifndef ISEKAI_HOST_FALCON_GATING_VARIABLE_H_
#define ISEKAI_HOST_FALCON_GATING_VARIABLE_H_

#include <functional>
#include <utility>

#include "glog/logging.h"

namespace isekai {

using GatingFunction = std::function<void()>;

// GatingVariable is a wrapper around primitive data types with common operators
// overloaded to call a GatingFunction whenever the underlying primitive value
// changes. In Isekai, this GatingFunction will invoke the recomputation of
// transmission eligibility for both connection scheduler and the retransmission
// scheduler.
template <typename T>
class GatingVariable {
 public:
  explicit GatingVariable() : function_(nullptr), value_(0) {}

  explicit GatingVariable(GatingFunction function)
      : function_(std::move(function)), value_(0) {}

  explicit GatingVariable(const T& value) : function_(nullptr), value_(value) {}

  explicit GatingVariable(GatingFunction function, const T& value)
      : function_(std::move(function)), value_(value) {}

  void SetGatingFunction(GatingFunction function) {
    function_ = std::move(function);
  }

  // Overload of all necessary operators.
  GatingVariable& operator=(const T& value) {
    if (value_ != value) {
      value_ = value;
      CheckAndCallGatingFunction();
    }
    return *this;
  }

  GatingVariable& operator+=(const T& rhs) {
    value_ += rhs;
    CheckAndCallGatingFunction();
    return *this;
  }

  GatingVariable& operator-=(const T& rhs) {
    value_ -= rhs;
    CheckAndCallGatingFunction();
    return *this;
  }

  GatingVariable& operator*=(const T& rhs) {
    value_ *= rhs;
    CheckAndCallGatingFunction();
    return *this;
  }

  GatingVariable& operator/=(const T& rhs) {
    value_ /= rhs;
    CheckAndCallGatingFunction();
    return *this;
  }

  GatingVariable& operator++() {  // Prefix increment.
    value_++;
    CheckAndCallGatingFunction();
    return *this;
  }

  T operator++(int) {  // Postfix increment.
    T old = value_++;
    CheckAndCallGatingFunction();
    return old;
  }

  GatingVariable& operator--() {  // Prefix decrement.
    value_--;
    CheckAndCallGatingFunction();
    return *this;
  }

  T operator--(int) {  // Postfix decrement.
    T old = value_--;
    CheckAndCallGatingFunction();
    return old;
  }

  operator T() const { return value_; }

  void UpdateWithoutCallingGatingFunction(const T& value) { value_ = value; }

 private:
  inline void CheckAndCallGatingFunction() {
    if (function_ != nullptr) {
      function_();
    } else {
      VLOG(2) << "Gating function not set.";
    }
  }

 private:
  GatingFunction function_;
  T value_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GATING_VARIABLE_H_

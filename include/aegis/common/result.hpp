/**
 * @file result.hpp
 * @brief Non-throwing monadic Result<T, E> wrapper for error propagation.
 * @author SMART AEGIS Engineering Team
 */

#ifndef AEGIS_COMMON_RESULT_HPP
#define AEGIS_COMMON_RESULT_HPP

#include <variant>
#include <utility>
#include <cassert>
#include "aegis/common/error_codes.hpp"

namespace aegis {
namespace common {

template <typename T, typename E = ErrorCode>
class Result {
public:
    static Result Ok(const T& val) { return Result(val); }
    static Result Ok(T&& val) { return Result(std::move(val)); }
    static Result Err(const E& err) { return Result(err); }

    bool IsOk() const noexcept { return std::holds_alternative<T>(m_storage); }
    bool IsErr() const noexcept { return std::holds_alternative<E>(m_storage); }

    const T& Value() const {
        assert(IsOk() && "Attempted to access Value on an Err Result");
        return std::get<T>(m_storage);
    }

    T& Value() {
        assert(IsOk() && "Attempted to access Value on an Err Result");
        return std::get<T>(m_storage);
    }

    const E& Error() const {
        assert(IsErr() && "Attempted to access Error on an Ok Result");
        return std::get<E>(m_storage);
    }

    T ValueOr(const T& defaultValue) const {
        return IsOk() ? Value() : defaultValue;
    }

private:
    explicit Result(const T& val) : m_storage(val) {}
    explicit Result(T&& val) : m_storage(std::move(val)) {}
    explicit Result(const E& err) : m_storage(err) {}

    std::variant<T, E> m_storage;
};

/**
 * @brief Specialization for Result<void, E>.
 */
template <typename E>
class Result<void, E> {
public:
    static Result Ok() { return Result(true, E{}); }
    static Result Err(const E& err) { return Result(false, err); }

    bool IsOk() const noexcept { return m_isOk; }
    bool IsErr() const noexcept { return !m_isOk; }

    const E& Error() const {
        assert(IsErr() && "Attempted to access Error on an Ok Result<void>");
        return m_error;
    }

private:
    Result(bool isOk, const E& err) : m_isOk(isOk), m_error(err) {}
    bool m_isOk{true};
    E m_error{};
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_RESULT_HPP

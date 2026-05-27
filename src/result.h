
#include <assert.h>
#include <optional>

#pragma once

template <typename T, typename E>
class result {
    std::optional<T> _value;
    E _error;

public:
    result(T value) : _value(value) {}
    result(E error) : _error(error) {}

    bool is_ok() const { return _value.has_value(); }

    T& value() {
        assert(is_ok());
        return _value.value();
    }

    const T& value() const {
        assert(is_ok());
        return _value.value();
    }

    E error() const {
        assert(!is_ok());
        return _error;
    }

    bool is_error(E error) const { return !is_ok() && _error == error; }
};

template <typename E>
class result<void, E> {
    bool _is_ok;
    E _error;

public:
    result() : _is_ok(true) {}
    result(E error) : _is_ok(false), _error(error) {}

    bool is_ok() const { return _is_ok; }

    void value() const { assert(is_ok()); }

    E error() const {
        assert(!is_ok());
        return _error;
    }

    bool is_error(E error) const { return !is_ok() && _error == error; }
};

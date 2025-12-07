#pragma once

#include "Context.h"
#include <string_view>
#include <memory>
#include <type_traits>

namespace obs {

class Span {
public:
    Span() = default;
    virtual ~Span() = default;
    
    Span(Span&&) noexcept = default;
    Span& operator=(Span&&) noexcept = default;
    Span(const Span&) = delete;
    Span& operator=(const Span&) = delete;
    
    virtual Span& attr(std::string_view key, std::string_view value) = 0;
    virtual Span& attr(std::string_view key, int64_t value) = 0;
    virtual Span& attr(std::string_view key, double value) = 0;
    
    template<typename T, typename = std::enable_if_t<std::is_same_v<T, bool>>>
    Span& attr(std::string_view key, T value) {
        return do_attr_bool(key, value);
    }
    
    virtual Span& set_error(std::string_view message) = 0;
    virtual Span& set_ok() = 0;
    virtual Span& event(std::string_view name) = 0;
    virtual Context context() const = 0;
    virtual bool is_recording() const = 0;

private:
    virtual Span& do_attr_bool(std::string_view key, bool value) = 0;
};

std::unique_ptr<Span> span(std::string_view name, const Context& ctx);
std::unique_ptr<Span> span(std::string_view name);

} // namespace obs

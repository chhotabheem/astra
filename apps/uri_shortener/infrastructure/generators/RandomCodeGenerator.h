/// @file RandomCodeGenerator.h
/// @brief Random code generator implementation

#pragma once

#include "domain/ports/ICodeGenerator.h"
#include <random>
#include <string>

namespace url_shortener::infrastructure {

/**
 * @brief Generates random alphanumeric short codes
 */
class RandomCodeGenerator : public domain::ICodeGenerator {
public:
    explicit RandomCodeGenerator(size_t length = 6)
        : m_length(length)
        , m_gen(std::random_device{}())
        , m_dist(0, m_chars.size() - 1) {}

    domain::ShortCode generate() override {
        std::string code;
        code.reserve(m_length);
        for (size_t i = 0; i < m_length; ++i) {
            code += m_chars[m_dist(m_gen)];
        }
        return domain::ShortCode::from_trusted(code);
    }

private:
    size_t m_length;
    std::mt19937 m_gen;
    std::uniform_int_distribution<size_t> m_dist;
    static constexpr std::string_view m_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
};

} // namespace url_shortener::infrastructure

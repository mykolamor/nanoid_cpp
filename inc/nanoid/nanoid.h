#pragma once

#include "nanoid/settings.h"
#include "nanoid/crypto_random.h"
#include <string>

namespace NANOID_NAMESPACE
{
    /// The default alphabet: URL-friendly symbols `A-Za-z0-9_-`.
    constexpr char url_alphabet[] = "_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    std::string generate();
    std::string generate(const std::string& alphabet);
    std::string generate(std::size_t size);
    std::string generate(const std::string& alphabet, std::size_t size);

    std::string generate(crypto_random_base& random);
    std::string generate(crypto_random_base& random, const std::string& alphabet);
    std::string generate(crypto_random_base& random, std::size_t size);
    std::string generate(crypto_random_base& random, const std::string& alphabet, std::size_t size);

    /// A reusable generator for one alphabet and default size.
    /// The alphabet is checked once, in the constructor.
    class custom_alphabet
    {
    private:
        std::string _alphabet;
        std::size_t _default_size;
        crypto_random_base* _random;
    public:
        /// Uses the default, OS random source.
        explicit custom_alphabet(std::string alphabet, std::size_t default_size = 21);
        /// Uses random, which must outlive this object.
        custom_alphabet(crypto_random_base& random, std::string alphabet, std::size_t default_size = 21);

        std::string operator()() const;
        std::string operator()(std::size_t size) const;
    };
}

#pragma once

#include "nanoid/settings.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

namespace NANOID_NAMESPACE
{
    class crypto_random_base
    {
    public:
        virtual void next_bytes(std::uint8_t* buffer, std::size_t size) = 0;
        virtual ~crypto_random_base() = default;
    };

    /// Reads from the operating system's cryptographically secure random source.
    ///
    /// One instance can be shared between threads.
    /// Throws std::system_error if the source fails.
    class os_random : public crypto_random_base
    {
    public:
        void next_bytes(std::uint8_t* buffer, std::size_t size) override;
    };

    namespace detail
    {
        // Number of bits needed to represent value.
        template <class T>
        constexpr unsigned bit_width(T value)
        {
            return value == 0 ? 0 : 1 + bit_width(static_cast<T>(value >> 1));
        }

        // UniformRandomBitGenerator to random bytes.
        template <class Generator>
        struct byte_source
        {
            using result_type = typename Generator::result_type;

            static constexpr result_type span()
            {
                return (Generator::max)() - (Generator::min)();
            }

            // span() + 1 is a power of two, so every output bit is uniformly random.
            static constexpr bool full_range()
            {
                return (span() & static_cast<result_type>(span() + 1u)) == 0;
            }

            // Whole bytes taken from each output, out of floor(log2(span() + 1)) random bits.
            static constexpr std::size_t bytes_per_call()
            {
                return (full_range() ? bit_width(span()) : bit_width(span()) - 1) / 8;
            }

            // Without a full range, the kept bytes are only uniform over outputs below
            // the largest multiple of 2^(8 * bytes_per_call()) that fits in span() + 1.
            static constexpr result_type limit()
            {
                return full_range()
                           ? span()
                           : static_cast<result_type>((span() + 1u) / (result_type(1) << (8 * bytes_per_call())) *
                                                      (result_type(1) << (8 * bytes_per_call())));
            }

            // Unsigned type exactly bytes_per_call() wide, if there is one.
            // clang-format off
            using word_type =
                typename std::conditional<sizeof(result_type) == bytes_per_call(), result_type,
                typename std::conditional<bytes_per_call() == 8, std::uint64_t,
                typename std::conditional<bytes_per_call() == 4, std::uint32_t,
                typename std::conditional<bytes_per_call() == 2, std::uint16_t, std::uint8_t>::type>::type>::type>::type;
            // clang-format on

            static constexpr bool exact_word()
            {
                return sizeof(word_type) == bytes_per_call();
            }
        };

        // True if a constructor call with Args would be a copy or move of Self.
        template <class Self, class... Args>
        struct is_self : std::false_type
        {
        };

        template <class Self, class Arg>
        struct is_self<Self, Arg> : std::is_same<Self, typename std::decay<Arg>::type>
        {
        };
    }

    /// Adapts a standard random engine (or any UniformRandomBitGenerator) to crypto_random_base.
    /// Only as secure as the engine.
    template <class UniformRandomBitGenerator>
    class engine_random : public crypto_random_base
    {
    private:
        using source = detail::byte_source<UniformRandomBitGenerator>;
        UniformRandomBitGenerator _generator;
    public:
        using result_type = typename UniformRandomBitGenerator::result_type;

        // std::is_unsigned for types like __int128.
        static_assert(static_cast<result_type>(-1) > static_cast<result_type>(0),
                      "engine_random requires a generator with an unsigned result_type.");
        static_assert(source::bytes_per_call() >= 1,
                      "engine_random requires a generator that produces at least 8 random bits per call.");

        engine_random& operator=(const engine_random& other) = delete;
        engine_random(const engine_random&) = delete;
        engine_random() : _generator() {}
        // Passes its arguments on to the engine constructor, e.g. a seed.
        // Disabled for engine_random itself, so copying reports the deleted copy constructor.
        template <class... Args, class = typename std::enable_if<!detail::is_self<engine_random, Args...>::value>::type>
        explicit engine_random(Args&&... args) : _generator(std::forward<Args>(args)...)
        {
        }

        static constexpr result_type(min)()
        {
            return (UniformRandomBitGenerator::min)();
        }

        static constexpr result_type(max)()
        {
            return (UniformRandomBitGenerator::max)();
        }

        result_type operator()()
        {
            return _generator();
        }

        void next_bytes(std::uint8_t* buffer, std::size_t size) override
        {
            const std::size_t step = source::bytes_per_call();
            std::size_t i = 0;
            for (; i + step <= size; i += step)
            {
                write_bytes(buffer + i, next_value(), std::integral_constant<bool, source::exact_word()>());
            }
            if (i == size) return;
            const result_type last = next_value();
            for (std::size_t k = 0; i < size; ++i, ++k)
            {
                buffer[i] = static_cast<std::uint8_t>(last >> (8 * k));
            }
        }

    private:
        // Next output shifted to start at 0, skipping outputs that would skew the bytes.
        result_type next_value()
        {
            while (true)
            {
                const result_type value = static_cast<result_type>(_generator() - (UniformRandomBitGenerator::min)());
                if (source::full_range() || value < source::limit()) return value;
            }
        }

        static void write_bytes(std::uint8_t* out, result_type value, std::true_type /*exact word*/)
        {
            const typename source::word_type word = static_cast<typename source::word_type>(value);
            std::memcpy(out, &word, sizeof(word));
        }

        static void write_bytes(std::uint8_t* out, result_type value, std::false_type /*exact word*/)
        {
            for (std::size_t k = 0; k < source::bytes_per_call(); ++k)
            {
                out[k] = static_cast<std::uint8_t>(value >> (8 * k));
            }
        }
    };
}
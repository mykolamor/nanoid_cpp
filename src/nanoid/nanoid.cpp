#include "nanoid/nanoid.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace NANOID_NAMESPACE
{
    namespace
    {
        const std::size_t default_size = 21;

        // Symbols generated per round of random bytes.
        // Bigger IDs take several rounds to not overflow the arithmetic below.
        const std::size_t max_round = 1024;
        // Bytes per round at most: ceil(1.6 * 256 * max_round / 129), 129 being the smallest cutoff.
        const std::size_t max_round_bytes = 3252;

        const std::string& default_alphabet()
        {
            static const std::string alphabet = url_alphabet;
            return alphabet;
        }

        crypto_random_base& default_random()
        {
            // Thread-safe, so sharing it between threads is safe.
            static os_random random;
            return random;
        }

        void check_alphabet(const std::string& alphabet)
        {
            if (alphabet.empty() || alphabet.size() > 256)
            {
                throw std::invalid_argument("alphabet must contain between 1 and 256 symbols.");
            }
        }

        void check_size(std::size_t size)
        {
            if (size == 0)
            {
                throw std::invalid_argument("size must be greater than zero.");
            }
        }

        std::string generate_checked(crypto_random_base& random, const std::string& alphabet, std::size_t size)
        {
            const std::size_t alphabet_size = alphabet.size();

            // Random bytes are 0-255. `byte % alphabet_size` alone would make some symbols more likely
            // (see https://github.com/ai/nanoid#security), so only bytes below the largest multiple
            // of the alphabet size are used.
            const std::size_t cutoff = 256 - 256 % alphabet_size;
            const bool power_of_two = cutoff == 256;

            // Secure random calls are expensive, so each round requests enough bytes to cover rejections in most cases.
            // Power-of-two alphabets reject nothing.
            // 1.6 is the factor upstream nanoid picked from benchmarks.
            const std::size_t round = std::min(size, max_round);
            const std::size_t step =
                power_of_two
                    ? round
                    : std::min(max_round_bytes,
                               static_cast<std::size_t>(std::ceil(1.6 * 256 * (double)round / (double)cutoff)));

            // Throws std::length_error for sizes that can't be allocated.
            std::string id(size, '_');
            std::uint8_t bytes[max_round_bytes];
            std::size_t count = 0;

            while (true)
            {
                random.next_bytes(bytes, step);

                for (std::size_t i = 0; i < step; i++)
                {
                    if (bytes[i] >= cutoff) continue;

                    // For power-of-two alphabets, `&` gives the same result as `%`, but faster.
                    id[count] = alphabet[power_of_two ? bytes[i] & (alphabet_size - 1) : bytes[i] % alphabet_size];
                    if (++count == size)
                    {
                        return id;
                    }
                }
            }
        }
    }
}

std::string NANOID_NAMESPACE::generate()
{
    return generate(default_random(), default_alphabet(), default_size);
}
std::string NANOID_NAMESPACE::generate(const std::string& alphabet)
{
    return generate(default_random(), alphabet, default_size);
}
std::string NANOID_NAMESPACE::generate(std::size_t size)
{
    return generate(default_random(), default_alphabet(), size);
}
std::string NANOID_NAMESPACE::generate(const std::string& alphabet, std::size_t size)
{
    return generate(default_random(), alphabet, size);
}

std::string NANOID_NAMESPACE::generate(crypto_random_base& random)
{
    return generate(random, default_alphabet(), default_size);
}
std::string NANOID_NAMESPACE::generate(crypto_random_base& random, const std::string& alphabet)
{
    return generate(random, alphabet, default_size);
}
std::string NANOID_NAMESPACE::generate(crypto_random_base& random, std::size_t size)
{
    return generate(random, default_alphabet(), size);
}
std::string NANOID_NAMESPACE::generate(crypto_random_base& random, const std::string& alphabet, std::size_t size)
{
    check_alphabet(alphabet);
    check_size(size);
    return generate_checked(random, alphabet, size);
}

NANOID_NAMESPACE::custom_alphabet::custom_alphabet(std::string alphabet, std::size_t default_size)
    : custom_alphabet(default_random(), std::move(alphabet), default_size)
{
}

NANOID_NAMESPACE::custom_alphabet::custom_alphabet(crypto_random_base& random, std::string alphabet,
                                                   std::size_t default_size)
    : _alphabet(std::move(alphabet)), _default_size(default_size), _random(&random)
{
    check_alphabet(_alphabet);
    check_size(_default_size);
}

std::string NANOID_NAMESPACE::custom_alphabet::operator()() const
{
    return generate_checked(*_random, _alphabet, _default_size);
}

std::string NANOID_NAMESPACE::custom_alphabet::operator()(std::size_t size) const
{
    check_size(size);
    return generate_checked(*_random, _alphabet, size);
}

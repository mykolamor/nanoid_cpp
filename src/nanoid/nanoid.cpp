#include "nanoid/nanoid.h"
#include <cmath>
#include <future>
#include <stdexcept>
#include <vector>

namespace NANOID_NAMESPACE
{
    namespace
    {
        const std::size_t default_size = 21;

        const std::string& default_alphabet()
        {
            static const std::string alphabet = "_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
            return alphabet;
        }

        crypto_random_base& default_random()
        {
            // Thread-safe, so sharing it between threads and std::async tasks is safe.
            static os_random random;
            return random;
        }

        // Smallest 2^k - 1 that is at least value.
        std::size_t mask_covering(std::size_t value)
        {
            std::size_t mask = 1;
            while (mask < value)
            {
                mask = (mask << 1) | 1;
            }
            return mask;
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

std::future<std::string> NANOID_NAMESPACE::generate_async()
{
    return std::async(std::launch::async, [] { return generate(); });
}
std::future<std::string> NANOID_NAMESPACE::generate_async(const std::string& alphabet)
{
    return std::async(std::launch::async, [alphabet] { return generate(alphabet); });
}
std::future<std::string> NANOID_NAMESPACE::generate_async(std::size_t size)
{
    return std::async(std::launch::async, [size] { return generate(size); });
}
std::future<std::string> NANOID_NAMESPACE::generate_async(const std::string& alphabet, std::size_t size)
{
    return std::async(std::launch::async, [alphabet, size] { return generate(alphabet, size); });
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
    if (alphabet.empty() || alphabet.size() > 256)
    {
        throw std::invalid_argument("alphabet must contain between 1 and 256 symbols.");
    }

    if (size == 0)
    {
        throw std::invalid_argument("size must be greater than zero.");
    }

    const std::size_t alphabet_size = alphabet.size();

    // See https://github.com/ai/nanoid/blob/main/README.md#security for
    // explanation why masking is used (`random % alphabet` is a common
    // mistake security-wise).
    const std::size_t mask = mask_covering(alphabet_size - 1);
    const auto step = static_cast<std::size_t>(std::ceil(1.6 * (double)mask * (double)size / (double)alphabet_size));

    std::string id(size, '_');
    std::vector<std::uint8_t> bytes(step);
    std::size_t count = 0;

    while (true)
    {
        random.next_bytes(bytes.data(), bytes.size());

        for (std::size_t i = 0; i < step; i++)
        {
            const std::size_t alphabet_index = bytes[i] & mask;
            if (alphabet_index >= alphabet_size) continue;

            id[count] = alphabet[alphabet_index];
            if (++count == size)
            {
                return id;
            }
        }
    }
}

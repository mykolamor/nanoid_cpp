#include "nanoid/nanoid.h"
#include <openssl/err.h>
#include <openssl/rand.h>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <string>

// Derives from crypto_random_base directly to fill the nanoid random buffer in one call.
// Seeding can happen in the constructor or globally beforehand.
class openssl_random : public NANOID_NAMESPACE::crypto_random_base
{
public:
    void next_bytes(std::uint8_t* buffer, std::size_t size) override
    {
        // RAND_bytes takes an int length, so large requests go in chunks.
        while (size > 0)
        {
            const int chunk = size > INT_MAX ? INT_MAX : (int)size;
            if (RAND_bytes(buffer, chunk) != 1)
            {
                char message[256];
                ERR_error_string_n(ERR_get_error(), message, sizeof(message));
                throw std::runtime_error(std::string("RAND_bytes failed: ") + message);
            }
            buffer += chunk;
            size -= (std::size_t)chunk;
        }
    }
};

int main()
{
    openssl_random random;
    try
    {
        for (int i = 0; i < 5; ++i)
        {
            std::cout << nanoid::generate(random, "abcdef", 10) << '\n';
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}

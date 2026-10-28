#include "nanoid/nanoid.h"
#include <exception>
#include <iostream>
#include <limits>
#include <string>

namespace
{
    const char* const usage = "Usage: nanoid [options]\n"
                              "\n"
                              "Generates a secure, URL-friendly unique ID.\n"
                              "\n"
                              "Options:\n"
                              "  -s, --size <n>            ID length (default: 21)\n"
                              "  -a, --alphabet <symbols>  Symbols to use (default: A-Za-z0-9_-)\n"
                              "  -v, --version             Print the version\n"
                              "  -h, --help                Print this help\n";

    // Accepts digits only, so "-1" is an error instead of wrapping around to a huge size.
    bool parse_size(const std::string& text, std::size_t& size)
    {
        if (text.empty()) return false;
        std::size_t value = 0;
        for (char c : text)
        {
            if (c < '0' || c > '9') return false;
            const std::size_t digit = static_cast<std::size_t>(c - '0');
            if (value > (std::numeric_limits<std::size_t>::max() - digit) / 10) return false;
            value = value * 10 + digit;
        }
        size = value;
        return true;
    }

    int fail(const std::string& message)
    {
        std::cerr << "nanoid: " << message << "\n\n" << usage;
        return 1;
    }
}

int main(int argc, char** argv)
{
    std::size_t size = 21;
    std::string alphabet = NANOID_NAMESPACE::url_alphabet;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
        {
            std::cout << usage;
            return 0;
        }
        if (arg == "-v" || arg == "--version")
        {
            std::cout << NANOID_VERSION_MAJOR << '.' << NANOID_VERSION_MINOR << '.' << NANOID_VERSION_PATCH << '\n';
            return 0;
        }

        const bool is_size = arg == "-s" || arg == "--size";
        const bool is_alphabet = arg == "-a" || arg == "--alphabet";
        if (!is_size && !is_alphabet) return fail("unknown option " + arg);
        if (i + 1 == argc) return fail(arg + " needs a value");

        const std::string value = argv[++i];
        if (is_size && !parse_size(value, size)) return fail("invalid size " + value);
        if (is_alphabet) alphabet = value;
    }

    try
    {
        std::cout << NANOID_NAMESPACE::generate(alphabet, size) << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr << "nanoid: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

#include <limits>
#include <map>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "nanoid/nanoid.h"

#if defined(__unix__) || defined(__APPLE__)
    #include <sys/wait.h>
    #include <unistd.h>
    #define NANOID_TEST_FORK
#endif

#define CATCH_CONFIG_MAIN
#include "catch2/catch.h"

namespace unit_tests
{
    static char const* default_dict = "_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static std::size_t default_size = 21;

    int clz32(int x)
    {
        const int numIntBits = sizeof(int) * 8; // compile time constant
        // do the smearing
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        // count the ones
        x -= x >> 1 & 0x55555555;
        x = (x >> 2 & 0x33333333) + (x & 0x33333333);
        x = (x >> 4) + x & 0x0f0f0f0f;
        x += x >> 8;
        x += x >> 16;
        return numIntBits - (x & 0x0000003f); // subtract # of 1s from 32
    }

    class predefined_random : public NANOID_NAMESPACE::crypto_random_base
    {
    public:
        using result_type = std::uint32_t;
    private:
        std::vector<std::uint8_t> _sequence;
    public:
        predefined_random& operator=(const predefined_random& other) = delete;
        predefined_random(const predefined_random&) = delete;
        explicit predefined_random(std::vector<std::uint8_t> vec) : _sequence(std::move(vec)) {}

        static constexpr result_type(min)()
        {
            return std::numeric_limits<std::uint32_t>::min();
        }

        static constexpr result_type(max)()
        {
            return std::numeric_limits<std::uint32_t>::max();
        }

        result_type operator()()
        {
            result_type res = _sequence[0];
            return res;
        }

        void next_bytes(std::uint8_t* buffer, std::size_t size) override
        {
            std::vector<std::uint8_t> bytes;
            for (std::size_t i = 0; i < size; i += _sequence.size())
            {
                bytes.insert(std::end(bytes), std::begin(_sequence),
                             std::begin(_sequence) + (long)std::min(size - i, _sequence.size()));
            }
            for (std::size_t i = 0; i < size; ++i)
            {
                // Have to revert order, because original uses reversed buffer, so double-reverse.
                buffer[i] = bytes[(size - i - 1)];
            }
        }
    };

    // Produces Bits random bits per call, in a T.
    // The n-th output has every byte equal to n, keeping the expected output independent of byte order.
    template <class T, unsigned Bits>
    class counting_engine
    {
    private:
        T _counter = 0;
    public:
        using result_type = T;
        static constexpr std::size_t bytes = Bits / 8;

        static constexpr result_type(min)()
        {
            return 0;
        }

        static constexpr result_type(max)()
        {
            return static_cast<T>(~T(0) >> (sizeof(T) * 8 - Bits));
        }

        result_type operator()()
        {
            ++_counter;
            return static_cast<T>(_counter * ((max)() / 0xFF));
        }
    };

    using counting_engine_24 = counting_engine<std::uint32_t, 24>;
    using counting_engine_32_in_64 = counting_engine<std::uint64_t, 32>;
    using counting_engine_64 = counting_engine<std::uint64_t, 64>;

#ifdef __SIZEOF_INT128__
    __extension__ typedef unsigned __int128 uint128;
    using counting_engine_128 = counting_engine<uint128, 128>;
#endif

    template <class Engine>
    void check_byte_layout(std::size_t size)
    {
        NANOID_NAMESPACE::engine_random<Engine> random;

        // Offset by one byte to exercise unaligned writes (with guard bytes on both sides).
        std::vector<std::uint8_t> storage(size + 2, 0xAA);
        random.next_bytes(storage.data() + 1, size);

        CHECK(storage.front() == 0xAA);
        CHECK(storage.back() == 0xAA);
        for (std::size_t i = 0; i < size; ++i)
        {
            INFO("size " << size << ", byte " << i);
            CHECK(storage[i + 1] == i / Engine::bytes + 1);
        }
    }

    template <class Engine>
    void check_flat_byte_distribution()
    {
        NANOID_NAMESPACE::engine_random<Engine> random(12345u);
        std::vector<std::uint8_t> bytes(1 << 22);
        random.next_bytes(bytes.data(), bytes.size());

        std::vector<std::size_t> counts(256, 0);
        for (auto b : bytes)
        {
            counts[b] += 1;
        }

        // 4 MiB gives 16,384 of each byte value.
        const double expected = (double)bytes.size() / 256.0;
        for (std::size_t value = 0; value < counts.size(); ++value)
        {
            INFO("byte " << value << " appeared " << counts[value] << " times, expected " << expected);
            CHECK((double)counts[value] == Approx(expected).epsilon(0.05));
        }
    }

    // Replays a fixed sequence of outputs in the range [Min, Max].
    template <class T, T Min, T Max>
    class sequence_engine
    {
    private:
        std::vector<T> _values;
        std::size_t _next = 0;
    public:
        using result_type = T;

        explicit sequence_engine(std::vector<T> values) : _values(std::move(values)) {}

        static constexpr result_type(min)()
        {
            return Min;
        }

        static constexpr result_type(max)()
        {
            return Max;
        }

        result_type operator()()
        {
            return _values[_next++ % _values.size()];
        }
    };
}

TEMPLATE_TEST_CASE("TestEngineRandomNextBytes", "", unit_tests::counting_engine_24,
                   unit_tests::counting_engine_32_in_64, unit_tests::counting_engine_64)
{
    unit_tests::check_byte_layout<TestType>(GENERATE(as<std::size_t>(), 0, 1, 2, 3, 4, 5, 7, 8, 9, 17, 35));
}

#ifdef __SIZEOF_INT128__
TEST_CASE("TestEngineRandomNextBytes128")
{
    unit_tests::check_byte_layout<unit_tests::counting_engine_128>(
        GENERATE(as<std::size_t>(), 0, 1, 15, 16, 17, 33, 35));
}
#endif

TEST_CASE("TestEngineRandomRejectsBiasedOutputs")
{
    // 300 possible outputs: 256 map onto one byte evenly,
    // the other 44 would favour bytes 0..43 and must be skipped.
    NANOID_NAMESPACE::engine_random<unit_tests::sequence_engine<std::uint32_t, 0, 299>> random(
        std::vector<std::uint32_t>{5, 256, 7, 299, 255, 0});

    std::vector<std::uint8_t> bytes(4);
    random.next_bytes(bytes.data(), bytes.size());
    CHECK(bytes == std::vector<std::uint8_t>{5, 7, 255, 0});
}

TEST_CASE("TestEngineRandomSubtractsMin")
{
    NANOID_NAMESPACE::engine_random<unit_tests::sequence_engine<std::uint32_t, 10, 265>> random(
        std::vector<std::uint32_t>{10, 265, 138});

    std::vector<std::uint8_t> bytes(3);
    random.next_bytes(bytes.data(), bytes.size());
    CHECK(bytes == std::vector<std::uint8_t>{0, 255, 128});
}

// ranlux24 gives 24 random bits per call, minstd_rand a range that isn't a power of two.
TEMPLATE_TEST_CASE("TestStandardEnginesFlatByteDistribution", "", std::ranlux24, std::minstd_rand)
{
    unit_tests::check_flat_byte_distribution<TestType>();
}

TEST_CASE("TestOsRandomSmallReads")
{
    // Many small reads with sizes that don't divide evenly into any buffer, so reads straddle refills.
    NANOID_NAMESPACE::os_random random;
    std::vector<std::size_t> counts(256, 0);
    std::vector<std::uint8_t> chunk(97);
    std::vector<std::uint8_t> previous;
    const std::size_t sizes[] = {1, 34, 97, 13, 64};
    for (std::size_t i = 0; i < 50000; ++i)
    {
        const std::size_t size = sizes[i % 5];
        random.next_bytes(chunk.data(), size);
        for (std::size_t k = 0; k < size; ++k)
        {
            counts[chunk[k]] += 1;
        }
        if (size == 97)
        {
            std::vector<std::uint8_t> current(chunk.begin(), chunk.end());
            REQUIRE(current != previous);
            previous = current;
        }
    }

    // 2,090,000 bytes give about 8,164 of each byte value.
    // 6% margin catches bugs like repeated or zeroed bytes.
    for (std::size_t value = 0; value < counts.size(); ++value)
    {
        INFO("byte " << value);
        CHECK((double)counts[value] == Approx(2090000.0 / 256.0).epsilon(0.06));
    }
}

#ifdef NANOID_TEST_FORK
TEST_CASE("TestForkedProcessesGenerateDifferentIds")
{
    // Generate once so the parent has buffered random bytes before forking.
    NANOID_NAMESPACE::generate();

    const std::size_t count = 300;
    int fds[2];
    REQUIRE(pipe(fds) == 0);

    const pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0)
    {
        close(fds[0]);
        std::string ids;
        for (std::size_t i = 0; i < count; ++i)
        {
            ids += NANOID_NAMESPACE::generate();
        }
        const ssize_t written = write(fds[1], ids.data(), ids.size());
        _exit(written == (ssize_t)ids.size() ? 0 : 1);
    }

    close(fds[1]);
    std::set<std::string> parent_ids;
    for (std::size_t i = 0; i < count; ++i)
    {
        parent_ids.insert(NANOID_NAMESPACE::generate());
    }

    std::string child_ids;
    char chunk[4096];
    ssize_t n;
    while ((n = read(fds[0], chunk, sizeof(chunk))) > 0)
    {
        child_ids.append(chunk, (std::size_t)n);
    }
    close(fds[0]);
    int status = 0;
    REQUIRE(waitpid(pid, &status, 0) == pid);
    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);
    REQUIRE(child_ids.size() == count * unit_tests::default_size);

    std::size_t shared = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        shared += parent_ids.count(child_ids.substr(i * unit_tests::default_size, unit_tests::default_size));
    }
    CHECK(shared == 0);
}
#endif

TEST_CASE("TestConcurrentGenerate")
{
    const std::size_t thread_count = 8;
    const std::size_t ids_per_thread = 5000;

    std::vector<std::vector<std::string>> results(thread_count);
    std::vector<std::thread> threads;
    for (std::size_t t = 0; t < thread_count; ++t)
    {
        threads.emplace_back([&, t] {
            for (std::size_t i = 0; i < ids_per_thread; ++i)
            {
                results[t].push_back(NANOID_NAMESPACE::generate());
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }

    std::set<std::string> unique;
    for (const auto& ids : results)
    {
        for (const auto& id : ids)
        {
            REQUIRE(id.size() == unit_tests::default_size);
            unique.insert(id);
        }
    }
    CHECK(unique.size() == thread_count * ids_per_thread);
}

TEST_CASE("TestDefault")
{
    std::string res = NANOID_NAMESPACE::generate();
    REQUIRE(res.size() == unit_tests::default_size);
}

TEST_CASE("TestCustomSize")
{
    const std::size_t size = 10;
    std::string res = NANOID_NAMESPACE::generate(size);
    REQUIRE(res.size() == size);
}

TEST_CASE("TestCustomAlphabet")
{
    const std::string alphabet = "1234abcd";

    char buff[256];
    snprintf(buff, sizeof(buff), "[%s]{%lu}$", alphabet.c_str(), unit_tests::default_size);
    std::string regex = buff;

    std::string res = NANOID_NAMESPACE::generate(alphabet);
    REQUIRE(res.size() == unit_tests::default_size);
    INFO(regex)
    REQUIRE_THAT(res, Catch::Matches(regex));
}

TEST_CASE("TestCustomAlphabetAndSize")
{
    const std::string alphabet = "1234abcd";
    const std::size_t size = 7;
    std::string res = NANOID_NAMESPACE::generate(alphabet, size);

    char buff[256];
    snprintf(buff, sizeof(buff), "[%s]{%lu}$", alphabet.c_str(), size);
    std::string regex = buff;

    REQUIRE(res.size() == size);
    INFO(regex)
    REQUIRE_THAT(res, Catch::Matches(regex));
}

TEST_CASE("TestSingleLetterAlphabet")
{
    const std::string alphabet = "a";
    const std::size_t size = 5;
    std::string res = NANOID_NAMESPACE::generate(alphabet, size);
    REQUIRE(res == "aaaaa");
}

TEST_CASE("TestPredefinedRandomSequence")
{
    const std::vector<std::uint8_t> seq{2, 255, 3, 7, 7, 7, 7, 7, 0, 1};
    unit_tests::predefined_random r(seq);

    using tp = std::pair<int, std::string>;

    auto tests = GENERATE(tp(4, "adca"), tp(18, "cbadcbadcbadcbadcc"));

    const std::string alphabet = "abcde";

    CHECK(NANOID_NAMESPACE::generate(r, alphabet, tests.first) == tests.second);
}

TEST_CASE("TestAsyncGenerate")
{
    std::string res = NANOID_NAMESPACE::generate_async().get();
    REQUIRE(res.size() == unit_tests::default_size);
}

TEST_CASE("TestGeneratesUrlFriendlyIDs")
{
    for (int i = 1; i <= 10; ++i)
    {
        std::string res = NANOID_NAMESPACE::generate();
        REQUIRE(res.size() == unit_tests::default_size);

        for (std::size_t j = 0; j < res.size(); ++j)
        {
            REQUIRE(res.find(res[j]) != std::string::npos);
        }
    }
}
TEST_CASE("TestHasNoCollisions")
{
    const int count = 100 * 1000;
    std::map<std::string, bool> dictUsed;
    int fails = 0;
    for (std::size_t dummy = 1; dummy <= count; ++dummy)
    {
        std::string result = NANOID_NAMESPACE::generate();
        auto it = dictUsed.find(result);
        fails += (it != dictUsed.end());
        dictUsed[result] = true;
    }
    CHECK(fails == 0);
}

TEST_CASE("TestFlatDistribution")
{
    const int count = 100 * 1000;
    std::map<char, int> chars;
    for (std::size_t dummy = 1; dummy <= count; ++dummy)
    {
        auto id = NANOID_NAMESPACE::generate();
        for (std::size_t i = 0; i < unit_tests::default_size; i++)
        {
            auto c = id[i];
            auto it = chars.find(c);
            if (it == chars.end())
            {
                chars[c] = 0;
            }
            chars[c] += 1;
        }
    }

    for (auto c : chars)
    {
        auto distribution =
            (double)(c.second * strlen(unit_tests::default_dict)) / (double)(count * unit_tests::default_size);
        REQUIRE(distribution == Approx(1.0).epsilon(0.05));
    }
}

TEST_CASE("TestMask")
{
    for (auto length = 1; length < 256; length++)
    {
        auto mask1 = (2 << (int)std::floor(std::log(length - 1) / std::log(2))) - 1;
        auto mask2 = (2 << (31 - unit_tests::clz32((length - 1) | 1))) - 1;
        REQUIRE(mask1 == mask2);
    }
}
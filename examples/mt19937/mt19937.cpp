#include "nanoid/nanoid.h"
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

// std::mt19937 is fast but NOT cryptographically secure.
// Use it only where IDs need not be unguessable,
// e.g. reproducible IDs in tests or simulations.

// Seeds the whole engine state.
static std::mt19937 make_seeded_mt19937()
{
    std::array<std::uint32_t, std::mt19937::state_size> seed_data;
    std::random_device device;
    std::generate(seed_data.begin(), seed_data.end(), std::ref(device));
    std::seed_seq seq(seed_data.begin(), seed_data.end());
    return std::mt19937(seq);
}

// A crypto_random<G> instance is not thread-safe, so every thread gets its own.
static nanoid::crypto_random<std::mt19937>& thread_random()
{
    thread_local nanoid::crypto_random<std::mt19937> random(make_seeded_mt19937());
    return random;
}

int main()
{
    std::vector<std::string> ids(4);
    std::vector<std::thread> threads;
    for (std::size_t t = 0; t < ids.size(); ++t)
    {
        threads.emplace_back([&ids, t] { ids[t] = nanoid::generate(thread_random()); });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }

    for (const auto& id : ids)
    {
        std::cout << id << '\n';
    }
    return 0;
}

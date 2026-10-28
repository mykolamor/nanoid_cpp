#include "nanoid/crypto_random.h"
#include <cerrno>
#include <system_error>

#if defined(_WIN32)
	#define NANOID_RANDOM_BCRYPT
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__ANDROID__)
	#define NANOID_RANDOM_ARC4RANDOM
#elif defined(__linux__) && defined(__has_include)
	#if __has_include(<sys/random.h>)
		#define NANOID_RANDOM_GETRANDOM
	#endif
#endif

#if defined(NANOID_RANDOM_BCRYPT)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
	#include <bcrypt.h>
#elif defined(NANOID_RANDOM_ARC4RANDOM)
	#include <stdlib.h>
#else
	#if defined(NANOID_RANDOM_GETRANDOM)
		#include <sys/random.h>
		#include <sys/mman.h>
		#if defined(MADV_WIPEONFORK) && !defined(NANOID_DISABLE_RANDOM_BUFFER)
			#define NANOID_RANDOM_WIPEONFORK_BUFFER
		#endif
	#endif
	#include <fcntl.h>
	#include <unistd.h>
#endif

namespace
{
#if !defined(NANOID_RANDOM_BCRYPT) && !defined(NANOID_RANDOM_ARC4RANDOM)
	void read_dev_urandom(std::uint8_t* buffer, std::size_t size)
	{
		int fd;
		do
		{
			fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
		} while (fd < 0 && errno == EINTR);
		if (fd < 0)
		{
			throw std::system_error(errno, std::generic_category(), "nanoid: cannot open /dev/urandom");
		}

		while (size > 0)
		{
			const ssize_t n = read(fd, buffer, size);
			if (n < 0 && errno == EINTR) continue;
			if (n <= 0)
			{
				const int error = n < 0 ? errno : EIO;
				close(fd);
				throw std::system_error(error, std::generic_category(), "nanoid: cannot read /dev/urandom");
			}
			buffer += n;
			size -= static_cast<std::size_t>(n);
		}
		close(fd);
	}
#endif

#if defined(NANOID_RANDOM_GETRANDOM)
	void read_getrandom(std::uint8_t* buffer, std::size_t size)
	{
		// getrandom may return fewer bytes than requested or be interrupted by a signal.
		while (size > 0)
		{
			const ssize_t n = getrandom(buffer, size, 0);
			if (n < 0)
			{
				if (errno == EINTR) continue;
				// Kernels older than 3.17 lack the syscall.
				if (errno == ENOSYS)
				{
					read_dev_urandom(buffer, size);
					return;
				}
				throw std::system_error(errno, std::generic_category(), "nanoid: getrandom failed");
			}
			buffer += n;
			size -= static_cast<std::size_t>(n);
		}
	}
#endif

#if defined(NANOID_RANDOM_WIPEONFORK_BUFFER)
	// Each thread fetches a page of random bytes at a time.
	// The page is marked MADV_WIPEONFORK for forked processes to refill the random buffer.
	class wipe_on_fork_buffer
	{
	private:
		struct page_header
		{
			std::size_t available; // Unused bytes, taken from the end of the data area.
		};

		std::uint8_t* _page = nullptr;
		std::size_t _page_size = 0;
		bool _unsupported = false;

		page_header& header() { return *reinterpret_cast<page_header*>(_page); }
		std::uint8_t* data() { return _page + sizeof(page_header); }
		std::size_t capacity() const { return _page_size - sizeof(page_header); }

		bool allocate()
		{
			const long page_size = sysconf(_SC_PAGESIZE);
			if (page_size <= 0) return false;
			void* page = mmap(nullptr, static_cast<std::size_t>(page_size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			if (page == MAP_FAILED) return false;
			// Kernels older than 4.14 reject MADV_WIPEONFORK. Without it buffering is not fork-safe.
			if (madvise(page, static_cast<std::size_t>(page_size), MADV_WIPEONFORK) != 0)
			{
				munmap(page, static_cast<std::size_t>(page_size));
				return false;
			}
			_page = static_cast<std::uint8_t*>(page);
			_page_size = static_cast<std::size_t>(page_size);
			return true;
		}

	public:
		wipe_on_fork_buffer() = default;
		wipe_on_fork_buffer(const wipe_on_fork_buffer&) = delete;
		wipe_on_fork_buffer& operator=(const wipe_on_fork_buffer&) = delete;

		~wipe_on_fork_buffer()
		{
			if (_page) munmap(_page, _page_size);
		}

		// Returns false if the request can't be served from the buffer.
		bool read(std::uint8_t* buffer, std::size_t size)
		{
			if (_unsupported) return false;
			if (!_page && !allocate())
			{
				_unsupported = true;
				return false;
			}
			if (size > capacity()) return false;

			if (header().available < size)
			{
				read_getrandom(data(), capacity());
				header().available = capacity();
			}

			std::uint8_t* const start = data() + capacity() - header().available;
			std::memcpy(buffer, start, size);
			// Don't keep bytes that were already handed out.
			std::memset(start, 0, size);
			header().available -= size;
			return true;
		}
	};

	thread_local wipe_on_fork_buffer thread_buffer;
#endif
}

void NANOID_NAMESPACE::os_random::next_bytes(std::uint8_t* buffer, std::size_t size)
{
	if (size == 0) return;
#if defined(NANOID_RANDOM_BCRYPT)
	// BCryptGenRandom takes a ULONG length, so large requests go in chunks.
	while (size > 0)
	{
		const ULONG chunk = size > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<ULONG>(size);
		const NTSTATUS status = BCryptGenRandom(nullptr, buffer, chunk, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		if (!BCRYPT_SUCCESS(status))
		{
			throw std::system_error(static_cast<int>(status), std::system_category(), "nanoid: BCryptGenRandom failed");
		}
		buffer += chunk;
		size -= chunk;
	}
#elif defined(NANOID_RANDOM_ARC4RANDOM)
	// Cannot fail.
	arc4random_buf(buffer, size);
#elif defined(NANOID_RANDOM_GETRANDOM)
	#if defined(NANOID_RANDOM_WIPEONFORK_BUFFER)
	if (thread_buffer.read(buffer, size)) return;
	#endif
	read_getrandom(buffer, size);
#else
	read_dev_urandom(buffer, size);
#endif
}

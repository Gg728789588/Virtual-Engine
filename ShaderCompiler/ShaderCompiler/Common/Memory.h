#pragma once
#include <cstdlib>
#include <stdint.h>
inline constexpr void* aligned_malloc(size_t size, size_t alignment)
{
	if (alignment & (alignment - 1))
	{
		return nullptr;
	}
	else
	{
		void* praw = malloc(sizeof(void*) + size + alignment);
		if (praw)
		{
			void* pbuf = reinterpret_cast<void*>(reinterpret_cast<size_t>(praw) + sizeof(void*));
			void* palignedbuf = reinterpret_cast<void*>((reinterpret_cast<size_t>(pbuf) | (alignment - 1)) + 1);
			(static_cast<void**>(palignedbuf))[-1] = praw;

			return palignedbuf;
		}
		else
		{
			return nullptr;
		}
	}
}

inline void aligned_free(void* palignedmem)
{
	free(reinterpret_cast<void*>((static_cast<void**>(palignedmem))[-1]));
}
//allocFunc:: void* operator()(size_t)
template <typename AllocFunc>
inline constexpr void* aligned_malloc(size_t size, size_t alignment, const AllocFunc& allocFunc)
{
	if (alignment & (alignment - 1))
	{
		return nullptr;
	}
	else
	{
		void* praw = allocFunc(sizeof(void*) + size + alignment);
		if (praw)
		{
			void* pbuf = reinterpret_cast<void*>(reinterpret_cast<size_t>(praw) + sizeof(void*));
			void* palignedbuf = reinterpret_cast<void*>((reinterpret_cast<size_t>(pbuf) | (alignment - 1)) + 1);
			(static_cast<void**>(palignedbuf))[-1] = praw;

			return palignedbuf;
		}
		else
		{
			return nullptr;
		}
	}
}

template <typename FreeFunc>
inline constexpr void* aligned_free(void* palignedmem, const FreeFunc& func)
{
	func(reinterpret_cast<void*>((static_cast<void**>(palignedmem))[-1]));
}

namespace vengine
{
	char* AllocateString(uint64_t& size, int32_t& poolIndex);
	void FreeString(void* ptr, int32_t poolIndex);
}

inline void* vengine_malloc(uint64_t size)
{
	size += 16;
	int32_t poolIndex = 0;
	char* resultString = vengine::AllocateString(size, poolIndex);
	uint64_t* poolIndexPtr = (uint64_t*)resultString;
	*poolIndexPtr = size - 16;
	poolIndexPtr += 1;
	*((int32_t*)poolIndexPtr) = poolIndex;
	poolIndexPtr += 1;
	return poolIndexPtr;
}

inline uint64_t vengine_memory_size(void* ptr)
{
	return *((uint64_t*)ptr - 2);
}

inline void vengine_free(void* ptr)
{
	uint64_t* poolIndexPtr = ((uint64_t*)ptr) - 1;
	vengine::FreeString(poolIndexPtr - 1, *((int32_t*)poolIndexPtr));
}

template <typename T, typename ... Args>
inline T* vengine_new(Args&&... args)
{
	T* tPtr = (T*)vengine_malloc(sizeof(T));
	new (tPtr)T(std::forward<Args>(args)...);
	return tPtr;
}

template <typename T>
inline void vengine_delete(T* ptr)
{
	struct TStr
	{
		T value;
		~TStr() {}
	};
	((TStr*)ptr)->~TStr();
	vengine_free(ptr);
}
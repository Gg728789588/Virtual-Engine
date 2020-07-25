#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <memory>
#include <type_traits>
#include "Memory.h"
template <typename T, bool useVEngineMalloc = true>
class ArrayList;
template <typename T>
class ArrayList<T, false>
{

private:

	T* arr;
	uint64_t mSize;
	uint64_t mCapacity;
public:
	T* Allocate(uint64_t& capacity)
	{
		capacity *= sizeof(T);
		if (capacity < 64)
			capacity = 64;
		else if (capacity < 128)
			capacity = 128;
		else if (capacity < 256)
			capacity = 256;
		T* ptr = (T*)malloc(capacity);
		capacity /= sizeof(T);
		return ptr;
	}
	void reserve(uint64_t newCapacity) noexcept
	{
		if (newCapacity <= mCapacity) return;
		T* newArr = Allocate(newCapacity);
		if (arr)
		{
			memcpy(newArr, arr, sizeof(T) * mSize);
			free(arr);
		}
		mCapacity = newCapacity;
		arr = newArr;
	}
	T* data() const noexcept { return arr; }
	uint64_t size() const noexcept { return mSize; }
	uint64_t capacity() const noexcept { return mCapacity; }
	struct Iterator
	{
		friend class ArrayList<T, false>;
	private:
		const ArrayList<T, false>* lst;
		uint64_t index;
		constexpr Iterator(const ArrayList<T, false>* lst, uint64_t index) noexcept : lst(lst), index(index) {}
	public:
		bool constexpr operator==(const Iterator& ite) const noexcept
		{
			return index == ite.index;
		}
		bool constexpr  operator!=(const Iterator& ite) const noexcept
		{
			return index != ite.index;
		}
		void operator++() noexcept
		{
			index++;
		}
		void operator++(int) noexcept
		{
			index++;
		}
		uint64_t GetIndex() const noexcept
		{
			return index;
		}
		Iterator constexpr  operator+(uint64_t value) const noexcept
		{
			return Iterator(lst, index + value);
		}
		Iterator constexpr  operator-(uint64_t value) const noexcept
		{
			return Iterator(lst, index - value);
		}
		Iterator& operator+=(uint64_t value) noexcept
		{
			index += value;
			return *this;
		}
		Iterator& operator-=(uint64_t value) noexcept
		{
			index -= value;
			return *this;
		}
		T* operator->() const noexcept
		{
#if defined(DEBUG) || defined(_DEBUG)
			if (index >= lst->mSize) throw "Out of Range!";
#endif
			return &(*lst).arr[index];
		}
		T& operator*() const noexcept
		{
#if defined(DEBUG) || defined(_DEBUG)
			if (index >= lst->mSize) throw "Out of Range!";
#endif
			return (*lst).arr[index];
		}

	};
	ArrayList(uint64_t mSize) noexcept : mSize(mSize), mCapacity(mSize)
	{
		arr = Allocate(mCapacity);

	}
	ArrayList(const ArrayList<T, false>& another) noexcept :
		mSize(another.mSize), mCapacity(another.mCapacity)
	{
		arr = Allocate(mCapacity);
		memcpy(arr, another.arr, sizeof(T) * mSize);
	}
	void operator=(const ArrayList<T, false>& another) noexcept
	{
		clear();
		resize(another.mSize);
		memcpy(arr, another.arr, sizeof(T) * mSize);
	}
	ArrayList() noexcept : mCapacity(0), mSize(0), arr(nullptr)
	{

	}
	~ArrayList() noexcept
	{
		if (arr) free(arr);
	}
	bool empty() const noexcept
	{
		return mSize == 0;
	}

	void SetZero() const noexcept
	{
		if (arr) memset(arr, 0, sizeof(T) * mSize);
	}



	template <typename ... Args>
	T& emplace_back(Args&&... args)
	{
		if (mSize >= mCapacity)
		{
			uint64_t newCapacity = mCapacity * 1.5 + 8;
			reserve(newCapacity);
		}
		auto& a = *(new (arr + mSize)T(std::forward<Args>(args)...));
		mSize++;
		return a;
	}

	void push_back(const T& value) noexcept
	{
		emplace_back(value);
	}

	void push_back_all(const T* values, uint64_t count)
	{
		if (mSize + count > mCapacity)
		{
			uint64_t newCapacity = mCapacity * 1.5 + 8;
			uint64_t values[2] = {
				mCapacity + 1, count + mSize };
			newCapacity = newCapacity > values[0] ? newCapacity : values[0];
			newCapacity = newCapacity > values[1] ? newCapacity : values[1];
			reserve(newCapacity);
		}
		memcpy(arr + mSize, values, count * sizeof(T));
		mSize += count;
	}

	void push_back_all(const std::initializer_list<T>& list)
	{
		push_back_all(list.begin(), list.size());
	}

	Iterator begin() const noexcept
	{
		return Iterator(this, 0);
	}
	Iterator end() const noexcept
	{
		return Iterator(this, mSize);
	}

	void erase(const Iterator& ite) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (ite.index >= mSize) throw "Out of Range!";
#endif
		if (ite.index < mSize - 1)
		{
			memmove(arr + ite.index, arr + ite.index + 1, (mSize - ite.index - 1) * sizeof(T));
		}
		mSize--;
	}
	void clear() noexcept
	{
		mSize = 0;
	}
	void dispose() noexcept
	{
		mSize = 0;
		mCapacity = 0;
		if (arr)
		{
			free(arr);
			arr = nullptr;
		}
	}
	void resize(uint64_t newSize) noexcept
	{
		reserve(newSize);
		mSize = newSize;
	}
	T& operator[](uint64_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	const T& operator[](uint64_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	T& operator[](uint32_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	const T& operator[](uint32_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	T& operator[](int32_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	const T& operator[](int32_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	T& operator[](int64_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	const T& operator[](int64_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
};
template <typename T>
class ArrayList<T, true>
{

private:

	T* arr;
	uint64_t mSize;
	uint64_t mCapacity;
	int32_t poolIndex;
public:
	T* Allocate(uint64_t& capacity, int32_t& poolIndex)
	{
		capacity *= sizeof(T);
		T* ptr = (T*)vengine::AllocateString(capacity, poolIndex);
		capacity /= sizeof(T);
		return ptr;
	}
	void reserve(uint64_t newCapacity) noexcept
	{
		if (newCapacity <= mCapacity) return;
		int32_t newPoolIndex = 0;
		T* newArr = Allocate(newCapacity, newPoolIndex);
		if (arr)
		{
			memcpy(newArr, arr, sizeof(T) * mSize);
			vengine::FreeString(arr, poolIndex);
		}
		mCapacity = newCapacity;
		poolIndex = newPoolIndex;
		arr = newArr;
	}
	T* data() const noexcept { return arr; }
	uint64_t size() const noexcept { return mSize; }
	uint64_t capacity() const noexcept { return mCapacity; }
	struct Iterator
	{
		friend class ArrayList<T, true>;
	private:
		const ArrayList<T, true>* lst;
		uint64_t index;
		constexpr Iterator(const ArrayList<T, true>* lst, uint64_t index) noexcept : lst(lst), index(index) {}
	public:
		bool constexpr operator==(const Iterator& ite) const noexcept
		{
			return index == ite.index;
		}
		bool constexpr  operator!=(const Iterator& ite) const noexcept
		{
			return index != ite.index;
		}
		void operator++() noexcept
		{
			index++;
		}
		void operator++(int) noexcept
		{
			index++;
		}
		Iterator constexpr  operator+(uint64_t value) const noexcept
		{
			return Iterator(lst, index + value);
		}
		Iterator constexpr  operator-(uint64_t value) const noexcept
		{
			return Iterator(lst, index - value);
		}
		Iterator& operator+=(uint64_t value) noexcept
		{
			index += value;
			return *this;
		}
		Iterator& operator-=(uint64_t value) noexcept
		{
			index -= value;
			return *this;
		}
		T* operator->() const noexcept
		{
#if defined(DEBUG) || defined(_DEBUG)
			if (index >= lst->mSize) throw "Out of Range!";
#endif
			return &(*lst).arr[index];
		}
		T& operator*() const noexcept
		{
#if defined(DEBUG) || defined(_DEBUG)
			if (index >= lst->mSize) throw "Out of Range!";
#endif
			return (*lst).arr[index];
		}

	};
	ArrayList(uint64_t mSize) noexcept : mSize(mSize), mCapacity(mSize)
	{
		arr = Allocate(mCapacity, poolIndex);

	}
	ArrayList(const ArrayList<T, true>& another) noexcept :
		mSize(another.mSize), mCapacity(another.mCapacity)
	{
		arr = Allocate(mCapacity, poolIndex);
		memcpy(arr, another.arr, sizeof(T) * mSize);
	}
	void operator=(const ArrayList<T, true>& another) noexcept
	{
		clear();
		resize(another.mSize);
		memcpy(arr, another.arr, sizeof(T) * mSize);
	}
	ArrayList() noexcept : mCapacity(0), mSize(0), arr(nullptr)
	{

	}
	~ArrayList() noexcept
	{
		if (arr) vengine::FreeString(arr, poolIndex);
	}
	bool empty() const noexcept
	{
		return mSize == 0;
	}

	void SetZero() const noexcept
	{
		if (arr) memset(arr, 0, sizeof(T) * mSize);
	}



	template <typename ... Args>
	T& emplace_back(Args&&... args)
	{
		if (mSize >= mCapacity)
		{
			uint64_t newCapacity = mCapacity * 1.5 + 8;
			reserve(newCapacity);
		}
		auto& a = *(new (arr + mSize)T(std::forward<Args>(args)...));
		mSize++;
		return a;
	}

	void push_back(const T& value) noexcept
	{
		emplace_back(value);
	}

	void push_back_all(const T* values, uint64_t count)
	{
		if (mSize + count > mCapacity)
		{
			uint64_t newCapacity = mCapacity * 1.5 + 8;
			uint64_t values[2] = {
				mCapacity + 1, count + mSize };
			newCapacity = newCapacity > values[0] ? newCapacity : values[0];
			newCapacity = newCapacity > values[1] ? newCapacity : values[1];
			reserve(newCapacity);
		}
		memcpy(arr + mSize, values, count * sizeof(T));
		mSize += count;
	}

	void push_back_all(const std::initializer_list<T>& list)
	{
		push_back_all(list.begin(), list.size());
	}

	Iterator begin() const noexcept
	{
		return Iterator(this, 0);
	}
	Iterator end() const noexcept
	{
		return Iterator(this, mSize);
	}

	void erase(const Iterator& ite) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (ite.index >= mSize) throw "Out of Range!";
#endif
		if (ite.index < mSize - 1)
		{
			memmove(arr + ite.index, arr + ite.index + 1, (mSize - ite.index - 1) * sizeof(T));
		}
		mSize--;
	}
	void clear() noexcept
	{
		mSize = 0;
	}
	void dispose() noexcept
	{
		mSize = 0;
		mCapacity = 0;
		if (arr)
		{
			vengine::FreeString(arr, poolIndex);
			arr = nullptr;
		}
	}
	void resize(uint64_t newSize) noexcept
	{
		reserve(newSize);
		mSize = newSize;
	}
	T& operator[](uint64_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	const T& operator[](uint64_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	T& operator[](uint32_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	const T& operator[](uint32_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	T& operator[](int32_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	const T& operator[](int32_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	T& operator[](int64_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
	const T& operator[](int64_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= mSize) throw "Out of Range!";
#endif
		return arr[index];
	}
};

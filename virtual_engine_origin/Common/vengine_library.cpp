#include "vstring.h"
#include "Pool.h"
#include <mutex>
#include "Runnable.h"
#include "Memory.h"
namespace vengine
{
	template<uint64_t size>
	struct DataString
	{
		alignas(16) char c[size];
	};
	Pool<DataString<32>> Bit32Pool(128, false);
	Pool<DataString<64>> Bit64Pool(64, false);
	Pool<DataString<128>> Bit128Pool(32, false);
	Pool<DataString<256>> Bit256Pool(16, false);
	std::mutex globalMtx32;
	std::mutex globalMtx64;
	std::mutex globalMtx128;
	std::mutex globalMtx256;
	char* AllocateString(uint64_t& size, int32_t& poolIndex)
	{
		char* ptr = nullptr;
		if (size <= 32)
		{
			poolIndex = 0;
			size = 32;
			ptr = (char*)Bit32Pool.New_Lock(globalMtx32);
		}
		else if (size <= 64)
		{
			poolIndex = 1;
			size = 64;
			ptr = (char*)Bit64Pool.New_Lock(globalMtx64);

		}
		else if (size <= 128)
		{
			poolIndex = 2;
			size = 128;
			ptr = (char*)Bit128Pool.New_Lock(globalMtx128);

		}
		else if (size <= 256)
		{
			poolIndex = 3;
			size = 256;
			ptr = (char*)Bit256Pool.New_Lock(globalMtx256);
		}
		else
		{
			poolIndex = -1;
			ptr = (char*)malloc(size);
		}
		return ptr;
	}
	wchar_t* AllocateString_Width(uint64_t& size, int32_t& poolIndex)
	{
		uint64_t widSize = size * 2;
		wchar_t* ptr = (wchar_t*)AllocateString(widSize, poolIndex);
		size = widSize / 2;
		return ptr;
	}

	void FreeString(void* ptr, int32_t poolIndex)
	{
		switch (poolIndex)
		{
		case 0:
		{
			Bit32Pool.Delete_Lock(globalMtx32, ptr);
		}
		break;
		case 1:
		{
			Bit64Pool.Delete_Lock(globalMtx64, ptr);
		}
		break;
		case 2:
		{
			Bit128Pool.Delete_Lock(globalMtx128, ptr);
		}
		break;
		case 3:
		{
			Bit256Pool.Delete_Lock(globalMtx256, ptr);
		}
		break;
		default:
			free(ptr);
			break;
		}
	}
#pragma region string
	string::string() noexcept
	{
		ptr = nullptr;
		capacity = 0;
		lenSize = 0;
	}
	string::~string() noexcept
	{
		if (ptr)
			FreeString(ptr, poolIndex);
	}
	string::string(char const* chr) noexcept
	{
		size_t size = strlen(chr);
		uint64_t newLenSize = size;
		size += 1;
		reserve(size);
		lenSize = newLenSize;
		memcpy(ptr, chr, size);
	}
	string::string(const char* chr, const char* chrEnd) noexcept
	{
		size_t size = chrEnd - chr;
		uint64_t newLenSize = size;
		size += 1;
		reserve(size);
		lenSize = newLenSize;
		memcpy(ptr, chr, size);
	}
	void string::reserve(uint64_t targetCapacity) noexcept
	{
		if (capacity >= targetCapacity) return;
		int32_t newPoolIndex = 0;
		char* newPtr = AllocateString(targetCapacity, newPoolIndex);
		if (ptr)
		{
			memcpy(newPtr, ptr, lenSize + 1);
			FreeString(ptr, poolIndex);
		}
		ptr = newPtr;
		poolIndex = newPoolIndex;
		capacity = targetCapacity;
	}
	string::string(const string& data) noexcept
	{
		if (data.ptr)
		{
			reserve(data.capacity);
			lenSize = data.lenSize;
			memcpy(ptr, data.ptr, lenSize);
			ptr[lenSize] = 0;
		}
		else
		{
			ptr = nullptr;
			capacity = 0;
			lenSize = 0;
		}
	}
	void string::resize(uint64_t newSize) noexcept
	{
		reserve(newSize + 1);
		lenSize = newSize;
		ptr[lenSize] = 0;
	}
	string::string(uint64_t size, char c) noexcept
	{
		reserve(size + 1);
		lenSize = size;
		memset(ptr, c, lenSize);
		ptr[lenSize] = 0;
	}
	string& string::operator=(const string& data) noexcept
	{
		if (data.ptr)
		{
			reserve(data.capacity);
			lenSize = data.lenSize;
			memcpy(ptr, data.ptr, lenSize);
			ptr[lenSize] = 0;

		}
		else
		{
			if (ptr)
				FreeString(ptr, poolIndex);
			ptr = nullptr;
			capacity = 0;
			lenSize = 0;
		}
		return *this;
	}
	string& string::operator=(const char* c) noexcept
	{
		size_t cSize = strlen(c);
		reserve(cSize + 1);
		lenSize = cSize;
		memcpy(ptr, c, cSize);
		ptr[lenSize] = 0;
		return *this;
	}

	string& string::operator=(char data) noexcept
	{
		lenSize = 1;
		ptr[0] = data;
		ptr[1] = 0;
		return *this;
	}

	string& string::operator+=(const string& str) noexcept
	{
		if (str.ptr)
		{
			uint64_t newCapacity = lenSize + str.lenSize + 1;
			reserve(newCapacity);
			memcpy(ptr + lenSize, str.ptr, str.lenSize);
			lenSize = newCapacity - 1;
			ptr[lenSize] = 0;
		}
		else
		{
			if (ptr)
				FreeString(ptr, poolIndex);
			ptr = nullptr;
			capacity = 0;
			lenSize = 0;
		}
		return *this;
	}
	string& string::operator+=(const char* str) noexcept
	{
		uint64_t newStrLen = strlen(str);
		uint64_t newCapacity = lenSize + newStrLen + 1;
		reserve(newCapacity);
		memcpy(ptr + lenSize, str, newStrLen);
		lenSize = newCapacity - 1;
		ptr[lenSize] = 0;
		return *this;
	}
	string& string::operator+=(char str) noexcept
	{
		static const uint64_t newStrLen = 1;
		uint64_t newCapacity = lenSize + newStrLen + 1;
		reserve(newCapacity);
		ptr[lenSize] = str;
		lenSize = newCapacity - 1;
		ptr[lenSize] = 0;
		return *this;
	}
	string::string(const string& a, const string& b) noexcept
	{
		if (!a.ptr && !b.ptr)
		{
			ptr = nullptr;
			capacity = 0;
			lenSize = 0;
		}
		else
		{
			uint64_t newLenSize = a.lenSize + b.lenSize;
			reserve(newLenSize + 1);
			lenSize = newLenSize;
			if (a.ptr)
				memcpy(ptr, a.ptr, a.lenSize);
			if (b.ptr)
				memcpy(ptr + a.lenSize, b.ptr, b.lenSize);
			ptr[lenSize] = 0;
		}
	}
	string::string(const string& a, const char* b) noexcept
	{
		size_t newLen = strlen(b);
		uint64_t newLenSize = a.lenSize + newLen;
		reserve(newLenSize + 1);
		lenSize = newLenSize;
		if (a.ptr)
			memcpy(ptr, a.ptr, a.lenSize);
		memcpy(ptr + a.lenSize, b, newLen);
		ptr[lenSize] = 0;
	}

	string::string(const char* a, const string& b) noexcept
	{
		size_t newLen = strlen(a);
		uint64_t newLenSize = b.lenSize + newLen;
		reserve(newLenSize + 1);
		lenSize = newLenSize;
		memcpy(ptr, a, newLen);
		if (b.ptr)
			memcpy(ptr + newLen, b.ptr, b.lenSize);
		ptr[lenSize] = 0;
	}

	string::string(const string& a, char b) noexcept
	{
		uint64_t newLenSize = a.lenSize + 1;
		reserve(newLenSize + 1);
		lenSize = newLenSize;
		if (a.ptr)
			memcpy(ptr, a.ptr, a.lenSize);
		ptr[a.lenSize] = b;
		ptr[newLenSize] = 0;
	}

	string::string(char a, const string& b) noexcept
	{
		uint64_t newLenSize = b.lenSize + 1;
		reserve(newLenSize + 1);
		lenSize = newLenSize;
		if (b.ptr)
			memcpy(ptr + 1, b.ptr, b.lenSize);
		ptr[0] = a;
		ptr[newLenSize] = 0;
	}

	char& string::operator[](uint64_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= lenSize)
			throw "Out of Range Exception!";
#endif
		return ptr[index];
	}
	void string::erase(uint64_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= lenSize)
			throw "Out of Range Exception!";
#endif
		memmove(ptr + index, ptr + index + 1, (lenSize - index));
		lenSize--;
	}
	char const& string::operator[](uint64_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= lenSize)
			throw "Out of Range Exception!";
#endif
		return ptr[index];
	}
	bool string::Equal(char const* str, uint64_t count) const noexcept
	{
		uint64_t bit64Count = count / 8;
		uint64_t leftedCount = count - bit64Count * 8;
		uint64_t const* value = (uint64_t const*)str;
		uint64_t const* oriValue = (uint64_t const*)ptr;
		for (uint64_t i = 0; i < bit64Count; ++i)
		{
			if (value[i] != oriValue[i]) return false;
		}
		char const* c = (char const*)(value + bit64Count);
		char const* oriC = (char const*)(oriValue + bit64Count);
		for (uint64_t i = 0; i < leftedCount; ++i)
		{
			if (c[i] != oriC[i]) return false;
		}
		return true;
	}
	std::ostream& operator << (std::ostream& out, const string& obj)
	{
		if (!obj.ptr) return out;
		for (uint64_t i = 0; i < obj.lenSize; ++i)
		{
			out << obj.ptr[i];
		}
		return out;
	}
	std::istream& operator >> (std::istream& in, string& obj)
	{
		char cArr[1024];
		in.getline(cArr, 1024);
		obj = cArr;
		return in;
	}
#pragma endregion
#pragma region wstring
	wstring::wstring() noexcept
	{
		ptr = nullptr;
		capacity = 0;
		lenSize = 0;
	}
	wstring::~wstring() noexcept
	{
		if (ptr)
			FreeString(ptr, poolIndex);
	}
	wstring::wstring(wchar_t const* chr) noexcept
	{
		size_t size = wstrLen(chr);
		uint64_t newLenSize = size;
		size += 1;
		reserve(size);
		lenSize = newLenSize;
		memcpy(ptr, chr, size * 2);
	}
	wstring::wstring(const wchar_t* wchr, const wchar_t* wchrEnd) noexcept
	{
		size_t size = wchrEnd - wchr;
		uint64_t newLenSize = size;
		size += 1;
		reserve(size);
		lenSize = newLenSize;
		memcpy(ptr, wchr, size * 2);
	}
	void wstring::reserve(uint64_t targetCapacity) noexcept
	{
		if (capacity >= targetCapacity) return;
		int32_t newPoolIndex = 0;
		targetCapacity *= 2;
		wchar_t* newPtr = (wchar_t*)AllocateString(targetCapacity, newPoolIndex);
		targetCapacity /= 2;
		if (ptr)
		{
			memcpy(newPtr, ptr, (lenSize + 1) * 2);
			FreeString(ptr, poolIndex);
		}
		ptr = newPtr;
		poolIndex = newPoolIndex;
		capacity = targetCapacity;
	}
	wstring::wstring(const wstring& data) noexcept
	{
		if (data.ptr)
		{
			reserve(data.capacity);
			lenSize = data.lenSize;
			memcpy(ptr, data.ptr, lenSize * 2);
			ptr[lenSize] = 0;
		}
		else
		{
			ptr = nullptr;
			capacity = 0;
			lenSize = 0;
		}
	}
	void wstring::resize(uint64_t newSize) noexcept
	{
		reserve(newSize + 1);
		lenSize = newSize;
		ptr[lenSize] = 0;
	}
	wstring::wstring(uint64_t size, wchar_t c) noexcept
	{
		reserve(size + 1);
		lenSize = size;
		memset(ptr, c, lenSize * 2);
		ptr[lenSize] = 0;
	}

	wstring::wstring(string const& str) noexcept
	{
		reserve(str.getCapacity());
		lenSize = str.size();
		for (uint64_t i = 0; i < lenSize; ++i)
			ptr[i] = str[i];
		ptr[lenSize] = 0;
	}
	wstring& wstring::operator=(const wstring& data) noexcept
	{
		if (data.ptr)
		{
			reserve(data.capacity);
			lenSize = data.lenSize;
			memcpy(ptr, data.ptr, lenSize * 2);
			ptr[lenSize] = 0;

		}
		else
		{
			if (ptr)
				FreeString(ptr, poolIndex);
			ptr = nullptr;
			capacity = 0;
			lenSize = 0;
		}
		return *this;
	}
	wstring& wstring::operator=(const wchar_t* c) noexcept
	{
		size_t cSize = wstrLen(c);
		reserve(cSize + 1);
		lenSize = cSize;
		memcpy(ptr, c, cSize * 2);
		ptr[lenSize] = 0;
		return *this;
	}

	wstring& wstring::operator=(wchar_t data) noexcept
	{
		lenSize = 1;
		ptr[0] = data;
		ptr[1] = 0;
		return *this;
	}

	wstring& wstring::operator+=(const wstring& str) noexcept
	{
		if (str.ptr)
		{
			uint64_t newCapacity = lenSize + str.lenSize + 1;
			reserve(newCapacity);
			memcpy(ptr + lenSize, str.ptr, str.lenSize * 2);
			lenSize = newCapacity - 1;
			ptr[lenSize] = 0;
		}
		else
		{
			if (ptr)
				FreeString(ptr, poolIndex);
			ptr = nullptr;
			capacity = 0;
			lenSize = 0;
		}
		return *this;
	}
	wstring& wstring::operator+=(const wchar_t* str) noexcept
	{
		uint64_t newStrLen = wstrLen(str);
		uint64_t newCapacity = lenSize + newStrLen + 1;
		reserve(newCapacity);
		memcpy(ptr + lenSize, str, newStrLen * 2);
		lenSize = newCapacity - 1;
		ptr[lenSize] = 0;
		return *this;
	}
	wstring& wstring::operator+=(wchar_t str) noexcept
	{
		static const uint64_t newStrLen = 1;
		uint64_t newCapacity = lenSize + newStrLen + 1;
		reserve(newCapacity);
		ptr[lenSize] = str;
		lenSize = newCapacity - 1;
		ptr[lenSize] = 0;
		return *this;
	}
	wstring::wstring(const wstring& a, const wstring& b) noexcept
	{
		if (!a.ptr && !b.ptr)
		{
			ptr = nullptr;
			capacity = 0;
			lenSize = 0;
		}
		else
		{
			uint64_t newLenSize = a.lenSize + b.lenSize;
			reserve(newLenSize + 1);
			lenSize = newLenSize;
			if (a.ptr)
				memcpy(ptr, a.ptr, a.lenSize * 2);
			if (b.ptr)
				memcpy(ptr + a.lenSize, b.ptr, b.lenSize * 2);
			ptr[lenSize] = 0;
		}
	}
	wstring::wstring(const wstring& a, const wchar_t* b) noexcept
	{
		size_t newLen = wstrLen(b);
		uint64_t newLenSize = a.lenSize + newLen;
		reserve(newLenSize + 1);
		lenSize = newLenSize;
		if (a.ptr)
			memcpy(ptr, a.ptr, a.lenSize * 2);
		memcpy(ptr + a.lenSize, b, newLen * 2);
		ptr[lenSize] = 0;
	}

	wstring::wstring(const wchar_t* a, const wstring& b) noexcept
	{
		size_t newLen = wstrLen(a);
		uint64_t newLenSize = b.lenSize + newLen;
		reserve(newLenSize + 1);
		lenSize = newLenSize;
		memcpy(ptr, a, newLen * 2);
		if (b.ptr)
			memcpy(ptr + newLen, b.ptr, b.lenSize * 2);
		ptr[lenSize] = 0;
	}

	wstring::wstring(const wstring& a, wchar_t b) noexcept
	{
		uint64_t newLenSize = a.lenSize + 1;
		reserve(newLenSize + 1);
		lenSize = newLenSize;
		if (a.ptr)
			memcpy(ptr, a.ptr, a.lenSize * 2);
		ptr[a.lenSize] = b;
		ptr[newLenSize] = 0;
	}

	wstring::wstring(wchar_t a, const wstring& b) noexcept
	{
		uint64_t newLenSize = b.lenSize + 1;
		reserve(newLenSize + 1);
		lenSize = newLenSize;
		if (b.ptr)
			memcpy(ptr + 1, b.ptr, b.lenSize * 2);
		ptr[0] = a;
		ptr[newLenSize] = 0;
	}

	wchar_t& wstring::operator[](uint64_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= lenSize)
			throw "Out of Range Exception!";
#endif
		return ptr[index];
	}
	void wstring::erase(uint64_t index) noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= lenSize)
			throw "Out of Range Exception!";
#endif
		memmove(ptr + index, ptr + index + 1, (lenSize - index) * 2);
		lenSize--;
	}
	wchar_t const& wstring::operator[](uint64_t index) const noexcept
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (index >= lenSize)
			throw "Out of Range Exception!";
#endif
		return ptr[index];
	}
	bool wstring::Equal(wchar_t const* str, uint64_t count) const noexcept
	{
		uint64_t bit64Count = count / 8;
		uint64_t leftedCount = count - bit64Count * 8;
		uint64_t const* value = (uint64_t const*)str;
		uint64_t const* oriValue = (uint64_t const*)ptr;
		for (uint64_t i = 0; i < bit64Count; ++i)
		{
			if (value[i] != oriValue[i]) return false;
		}
		wchar_t const* c = (wchar_t const*)(value + bit64Count);
		wchar_t const* oriC = (wchar_t const*)(oriValue + bit64Count);
		for (uint64_t i = 0; i < leftedCount; ++i)
		{
			if (c[i] != oriC[i]) return false;
		}
		return true;
	}

#pragma endregion
}
namespace RunnableGlobal
{
	void* AllocateMemory(uint64_t& size, int32_t& poolIndex)
	{
		return vengine::AllocateString(size, poolIndex);
	}
	void FreeMemory(void* ptr, int32_t poolIndex)
	{
		vengine::FreeString(ptr, poolIndex);
	}
}
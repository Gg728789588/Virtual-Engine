#pragma once
#include <atomic>
#include <mutex>
#include "Runnable.h"
#include "MetaLib.h"
#include "ArrayList.h"
#include <assert.h>
#include "vector.h"
class PtrLink;

class MObject
{
	friend class PtrLink;
private:
	Runnable<void(MObject*)> disposeFunc;
	vengine::vector<Runnable<void(MObject*)>> disposeFuncs;
	static std::atomic<uint32_t> CurrentID;
	uint32_t instanceID;
protected:
	MObject()
	{
		instanceID = ++CurrentID;
	}
public:
	void AddEventBeforeDispose(const Runnable<void(MObject*)>& func);
	uint32_t GetInstanceID() const { return instanceID; }
	virtual ~MObject() noexcept;
};


class PtrLink;
class PtrWeakLink;
struct LinkHeap
{
	friend class PtrLink;
	friend class PtrWeakLink;
private:
	static void Resize() noexcept;
	void(*disposor)(void*);
	std::atomic<int32_t> refCount;
	std::atomic<int32_t> looseRefCount;
	static ArrayList<LinkHeap*> heapPtrs;
	static std::mutex mtx;
	static LinkHeap* GetHeap(void* obj, void(*disp)(void*)) noexcept;
	static void ReturnHeap(LinkHeap* value) noexcept;
public:
	void* ptr;
};

class VEngine;
class PtrWeakLink;
class PtrLink
{
	friend class VEngine;
	friend class PtrWeakLink;
private:
	template <typename T>
	struct TypeCollector
	{
		T t;
	};
public:
	LinkHeap* heapPtr;
	PtrLink() noexcept : heapPtr(nullptr)
	{

	}
	void Dispose() noexcept;
	template <typename T>
	PtrLink(T* obj) noexcept
	{
		void(*disposor)(void*) = [](void* ptr)->void
		{
			delete ((T*)ptr);
		};
		heapPtr = LinkHeap::GetHeap(obj, disposor);
	}
	template <typename T>
	PtrLink(T* obj, bool) noexcept
	{
		void(*disposor)(void*) = [](void* ptr)->void
		{
			((TypeCollector<T>*)ptr)->~TypeCollector();
		};
		heapPtr = LinkHeap::GetHeap(obj, disposor);
	}
	PtrLink(const PtrLink& p) noexcept;
	void operator=(const PtrLink& p) noexcept;
	inline PtrLink(const PtrWeakLink& link) noexcept;
	
	void Destroy() noexcept;
	~PtrLink() noexcept
	{
		Dispose();
	}
};
class PtrWeakLink
{
public:
	LinkHeap* heapPtr;
	PtrWeakLink() noexcept : heapPtr(nullptr)
	{

	}

	void Dispose() noexcept;
	PtrWeakLink(const PtrLink& p) noexcept;
	PtrWeakLink(const PtrWeakLink& p) noexcept;
	void operator=(const PtrLink& p) noexcept;
	void operator=(const PtrWeakLink& p) noexcept;
	void Destroy() noexcept;

	~PtrWeakLink() noexcept
	{
		Dispose();
	}
};


template <typename T>
class ObjWeakPtr;
template <typename T>
class ObjectPtr
{
private:
	friend class ObjWeakPtr<T>;
	PtrLink link;
	inline constexpr ObjectPtr(T* ptr) noexcept :
		link(ptr)
	{

	}
	inline constexpr ObjectPtr(T* ptr, bool) noexcept :
		link(ptr, false)
	{

	}
public:
	constexpr ObjectPtr(const PtrLink& link) noexcept : link(link)
	{
	}
	inline constexpr ObjectPtr() noexcept :
		link() {}
	inline constexpr ObjectPtr(std::nullptr_t) noexcept : link()
	{

	}
	inline constexpr ObjectPtr(const ObjectPtr<T>& ptr) noexcept :
		link(ptr.link)
	{

	}
	inline constexpr ObjectPtr(const ObjWeakPtr<T>& ptr) noexcept;
	static ObjectPtr<T> MakePtr(T* ptr) noexcept
	{
		return ObjectPtr<T>(ptr);
	}
	static ObjectPtr<T> MakePtrNoMemoryFree(T* ptr) noexcept
	{
		return ObjectPtr<T>(ptr, false);
	}
	static ObjectPtr<T> MakePtr(ObjectPtr<T>) noexcept = delete;


	inline constexpr operator bool() const noexcept
	{
		return link.heapPtr != nullptr && link.heapPtr->ptr != nullptr;
	}

	inline constexpr operator T* () const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return (T*)link.heapPtr->ptr;
	}

	inline constexpr void Destroy() noexcept
	{
		link.Destroy();
	}

	template<typename F>
	inline constexpr ObjectPtr<F> CastTo() const noexcept
	{
		return ObjectPtr<F>(link);
	}
	inline constexpr void operator=(const ObjWeakPtr<T>& other) noexcept;
	inline constexpr void operator=(const ObjectPtr<T>& other) noexcept
	{
		link = other.link;
	}

	inline constexpr void operator=(T* other) noexcept = delete;
	inline constexpr void operator=(void* other) noexcept = delete;
	inline constexpr void operator=(std::nullptr_t t) noexcept
	{
		link.Dispose();
	}

	inline constexpr T* operator->() const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return (T*)link.heapPtr->ptr;
	}

	inline constexpr T& operator*() noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return *(T*)link.heapPtr->ptr;
	}

	inline constexpr T const& operator*() const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return *(T*)link.heapPtr->ptr;
	}

	inline constexpr bool operator==(const ObjectPtr<T>& ptr) const noexcept
	{
		return link.heapPtr == ptr.link.heapPtr;
	}
	inline constexpr bool operator!=(const ObjectPtr<T>& ptr) const noexcept
	{
		return link.heapPtr != ptr.link.heapPtr;
	}
};

template <typename T>
class ObjWeakPtr
{
private:
	friend class ObjectPtr<T>;
	PtrWeakLink link;
public:
	inline constexpr ObjWeakPtr() noexcept :
		link() {}
	inline constexpr ObjWeakPtr(std::nullptr_t) noexcept : link()
	{

	}
	inline constexpr ObjWeakPtr(const ObjWeakPtr<T>& ptr) noexcept :
		link(ptr.link)
	{

	}
	inline constexpr ObjWeakPtr(const ObjectPtr<T>& ptr) noexcept :
		link(ptr.link)
	{

	}

	inline constexpr operator bool() const noexcept
	{
		return link.heapPtr != nullptr && link.heapPtr->ptr != nullptr;
	}

	inline constexpr operator T* () const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return (T*)link.heapPtr->ptr;
	}

	inline constexpr void Destroy() noexcept
	{
		link.Destroy();
	}

	template<typename F>
	inline constexpr ObjWeakPtr<F> CastTo() const noexcept
	{
		return ObjWeakPtr<F>(link);
	}

	inline constexpr void operator=(const ObjWeakPtr<T>& other) noexcept
	{
		link = other.link;
	}

	inline constexpr void operator=(const ObjectPtr<T>& other) noexcept
	{
		link = other.link;
	}

	inline constexpr void operator=(T* other) noexcept = delete;
	inline constexpr void operator=(void* other) noexcept = delete;
	inline constexpr void operator=(std::nullptr_t t) noexcept
	{
		link.Dispose();
	}

	inline constexpr T* operator->() const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return (T*)link.heapPtr->ptr;
	}

	inline constexpr T& operator*() noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return *(T*)link.heapPtr->ptr;
	}
	inline constexpr T const& operator*() const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return *(T*)link.heapPtr->ptr;
	}

	inline constexpr bool operator==(const ObjWeakPtr<T>& ptr) const noexcept
	{
		return link.heapPtr == ptr.link.heapPtr;
	}
	inline constexpr bool operator!=(const ObjWeakPtr<T>& ptr) const noexcept
	{
		return link.heapPtr != ptr.link.heapPtr;
	}

};
template<typename T>
inline constexpr ObjectPtr<T>::ObjectPtr(const ObjWeakPtr<T>& ptr) noexcept :
	link(ptr.link)
{

}
template<typename T>
inline constexpr void ObjectPtr<T>::operator=(const ObjWeakPtr<T>& other) noexcept
{
	link = other.link;
}



inline PtrLink::PtrLink(const PtrWeakLink& p) noexcept
{
	if (p.heapPtr && p.heapPtr->ptr) {
		++p.heapPtr->refCount;
		++p.heapPtr->looseRefCount;
		heapPtr = p.heapPtr;
	}
	else
	{
		heapPtr = nullptr;
	}
}
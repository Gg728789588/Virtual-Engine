#pragma once
#include <type_traits>
#include <typeinfo>
#include <stdint.h>
#include "HashMap.h"
using UINT = uint32_t;
template <typename K, typename V>
class Dictionary
{
public:
	struct KVPair
	{
		K key;
		V value;
	};
	
	HashMap<K, UINT> keyDicts;
	vengine::vector<KVPair> values;
	void Reserve(UINT capacity);
	V* operator[](const K& key);

	void Add(const K& key, const V& value);
	void Remove(const K& key);

	void Clear();
	Dictionary() {}
	Dictionary(uint64_t capacity) :
		keyDicts(capacity)
	{
		values.reserve(capacity);
	}
};
template <typename K, typename V>
void Dictionary<K, V>::Reserve(UINT capacity)
{
	keyDicts.Reserve(capacity);
	values.reserve(capacity);
}
template <typename K, typename V>
V* Dictionary<K, V>::operator[](const K& key)
{
	auto ite = keyDicts.Find(key);
	if (ite == keyDicts.End()) return nullptr;
	return &values[ite.Value()].value;
}
template <typename K, typename V>
void Dictionary<K, V>::Add(const K& key, const  V& value)
{
	keyDicts.Insert(key, values.size());
	values.push_back({ (key), (value) });
}
template <typename K, typename V>
void Dictionary<K, V>::Remove(const K& key)
{
	auto ite = keyDicts.Find(key);
	if (ite == keyDicts.End()) return;
	KVPair& p = values[ite.Value()];
	p = values[values.size() - 1];
	keyDicts[p.key] = ite.Value();
	values.erase(values.end() - 1);
	keyDicts.Remove(ite);
}
template <typename K, typename V>
void Dictionary<K, V>::Clear()
{
	keyDicts.Clear();
	values.clear();
}

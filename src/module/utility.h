#ifndef IG_MODULE_UTILITY_H
#define IG_MODULE_UTILITY_H

// something small, independent (except for STL) and useful

#include <cassert>
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <functional>
#include "pair_range.hpp"
#include "range_all.hpp"

using std::string;
using std::vector;
using std::unordered_map;
using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;

// unreachable code
#define assert_sentinel do { assert(false); throw; } while(false)

#ifdef _DEBUG
# define DbgArea /* empty */
#else
# define DbgArea if ( false )
#endif

// optional<T&>
template<typename T> using optional_ref = T*;

// wrap raw ptr in shared_ptr
template<typename T>
static shared_ptr<T> shared_ptr_from_rawptr(T* p) { return shared_ptr<T>(p, [](void const*) {}); }

// Find key and return the value; Or insert the key with the value of f().
template<typename Map, typename Key, typename Value = decltype(std::declval<Map>()[std::declval<Key>()]), typename Fun>
static Value& map_find_or_insert(Map& m, Key const& key, Fun&& f)
{
	auto lb = m.lower_bound(key);
	if ( lb != m.end() && !(m.key_comp()(key, lb->first)) ) {
		return lb->second;
	} else {
		return m.emplace_hint(lb, key, std::forward<Fun>(f)())->second;
	}
}

// Check the first range begins with the second range.
template<typename Iter, typename IterSub>
static bool begins_with(Iter&& begin, Iter&& end, IterSub&& begin_sub, IterSub&& end_sub)
{
	auto it = begin;
	auto it_sub = begin_sub;
	for ( ; it != end && it_sub != end_sub; ++it, ++it_sub ) {
		if ( *it != *it_sub ) return false;
	}
	return true;
}

#endif

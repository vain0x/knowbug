
#pragma once

#include <iterator>

#define RANGE_ALL(_X) range::begin((_X)), range::end((_X))

namespace range {
namespace detail {

using std::begin;
using std::end;

struct begin_fn
{
	template<typename R>
	auto operator()(R&& rng) const
		-> decltype(begin(std::forward<R>(rng)))
	{
		return begin(std::forward<R>(rng));
	}
};

struct end_fn
{
	template<typename R>
	auto operator()(R&& rng) const
		-> decltype(end(std::forward<R>(rng)))
	{
		return end(std::forward<R>(rng));
	}
};

} //namespace detail

static auto const begin = detail::begin_fn {};
static auto const end   = detail::end_fn {};

} //namespace range

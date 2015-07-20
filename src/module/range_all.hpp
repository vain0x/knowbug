#ifndef IG_MODULE_RANGE_ALL_HPP
#define IG_MODULE_RANGE_ALL_HPP

#include <iterator>

// range parameter for STL algorhytm
#define RANGE_ALL(_X) range::begin((_X)), range::end((_X))

namespace Detail {

using std::begin;
using std::end;

struct begin_fn
{
	template<typename R> auto operator()(R&& rng) const -> decltype(begin(std::forward<R>(rng)))
	{
		return begin(std::forward<R>(rng));
	}
};

struct end_fn
{
	template<typename R> auto operator()(R&& rng) const -> decltype(end(std::forward<R>(rng)))
	{
		return end(std::forward<R>(rng));
	}
};

}//namespace Detail

namespace range {
	static Detail::begin_fn const begin {};
	static Detail::end_fn const end {};
}

#endif

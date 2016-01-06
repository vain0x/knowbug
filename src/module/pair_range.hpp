#ifndef IG_ITERATOR_PAIR_RANGE_HPP
#define IG_ITERATOR_PAIR_RANGE_HPP

#include <iterator>

template<typename Iter>
struct pair_range
	: public std::pair<Iter, Iter>
{
private:
	using base_type = std::pair<Iter, Iter>;

public:
	template<typename... Args> pair_range(Args&&... args)
		: base_type(std::forward<Args>(args)...)
	{ }

	auto begin() -> Iter&  { return first; }
	auto   end() -> Iter&  { return second; }
	auto begin() const -> Iter const& { return first; }
	auto   end() const -> Iter const& { return second; }
};

template<typename Iter>
static auto make_pair_range(Iter&& begin, Iter&& end) -> pair_range<Iter> {
	return pair_range<Iter>(std::forward<Iter>(begin), std::forward<Iter>(end));
}

template<typename Container>
static auto make_pair_range(Container& con) -> pair_range<decltype(con.begin())> {
	return make_pair_range(con.begin(), con.end());
}

template<typename Container>
static auto make_pair_range(Container const& con) -> pair_range<decltype(con.begin())> {
	return make_pair_range(con.begin(), con.end());
}

#endif

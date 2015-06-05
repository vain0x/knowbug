#ifndef IG_MAP_ITERATOR_H
#define IG_MAP_ITERATOR_H

#include <iterator>
#include <functional>

//------------------------------------------------
// iterator_range
//
// wait for Range library
//------------------------------------------------
template<typename TIter>
class iterator_range
{
private:
	TIter begin_, end_;

	using traits = std::iterator_traits<TIter>;
	using self_t = iterator_range<TIter>;
public:
	TIter const& begin() const { return begin_; }
	TIter const& end() const { return end_; }

//	iterator_range() : begin_(), end_() { }
	iterator_range(self_t const& src) : iterator_range(src.begin_, src.end_) { }
	iterator_range(self_t&& src) : iterator_range(std::move(src.begin_), std::move(src.end_)) { }

	template<typename TBeginInit, typename TEndInit>
	iterator_range(TBeginInit&& begin, TEndInit&& end)
		: begin_(std::forward<TBeginInit>(begin)), end_(std::forward<TEndInit>(end))
	{ }
};

//------------------------------------------------
// iterator of mapping
//
// TFunc の返値が non-const への参照型なら、std::forward_iterator 以上になる。
// todo: そのためのテンプレート特殊化
// remark. 関数子が異なっていても base が等しければ等しいとみなされる。注意。
// ラムダ関数が一般に比較可能でないため。
//------------------------------------------------
template<typename TIter, typename TFunc,
	typename TIterTag = std::input_iterator_tag,
	typename TIterTraits = std::iterator_traits<TIter>,
	typename TElem = std::result_of_t<TFunc(typename TIterTraits::reference)>,
	typename TDistance = typename TIterTraits::difference_type
>
class mapped_iterator
	: public std::iterator<TIterTag, TElem, TDistance, std::remove_reference_t<TElem>*>
{
private:
	TIter base_;
	TFunc func_;

	using self_t = mapped_iterator<TIter, TFunc, TIterTag, TIterTraits, TElem, TDistance>;

public:
	mapped_iterator() : base_(), func_() { }

	mapped_iterator(self_t const& src)
		: mapped_iterator(src.base_, src.func_)
	{ }
	mapped_iterator(self_t&& src)
		: mapped_iterator(std::move(src.base_), std::move(src.func_))
	{ }
	template<typename TIterInit, typename TFuncInit>
	mapped_iterator(TIterInit&& base, TFuncInit&& func)
		: base_(std::forward<TIterInit>(base))
		, func_(std::forward<TFuncInit>(func))
	{ }

	self_t& operator=(self_t const& rhs) {
		~mapped_iterator(); new(this) self_t(rhs); return *this;
	}

	// operators as iterator
	value_type operator*() const { return func_(*base_); }
	
	self_t& operator++() { ++base_; return *this; }
	self_t& operator--() { --base_; return *this; }
	self_t operator++(int) { auto bak(*this); ++(*this); return std::move(bak); }
	self_t operator--(int) { auto bak(*this); --(*this); return std::move(bak); }

	bool operator==(self_t const& rhs) const { return (/*func_ == rhs.func_ &&*/ base_ == rhs.base_); }
	bool operator!=(self_t const& rhs) const { return !(*this == rhs); }

	bool operator< (self_t const& rhs) const { return base_ < rhs.base_; }
	bool operator<=(self_t const& rhs) const { return compare(*this, rhs) <= 0; }
	bool operator> (self_t const& rhs) const { return compare(*this, rhs) >  0; }
	bool operator>=(self_t const& rhs) const { return compare(*this, rhs) >= 0; }

	self_t operator+(difference_type n) const { return self_t(base_ + n, func_); }
	self_t operator-(difference_type n) const { return self_t(base_ - n, func_); }
	self_t& operator+=(difference_type n) { base_ += n; return *this; }
	self_t& operator-=(difference_type n) { base_ -= n; return *this; }
	value_type operator[](difference_type n) const { return *(*this + n); }
	difference_type operator-(self_t const& rhs) const { return base_ - rhs.base_; }

	std::remove_reference_t<value_type>* operator->() const {
		static_assert(std::is_lvalue_reference<value_type>::value, "operator [] can be used only when map returns lvalue-reference.");
		return std::addressof(*(*this));
	}
private:
	int compare(self_t const& rhs) const { return (lhs == rhs ? 0 : (lhs < rhs ? 1 : -1)); }

public:
	// others
	TIter const& base() const { return base_; }
	TFunc const& func() const { return func_; }
};

template<typename TIter, typename TFunc>
static auto make_mapped_iterator(TIter iter, TFunc func) -> mapped_iterator<TIter, TFunc>
{ return mapped_iterator<TIter, TFunc>(iter, func); }

template<typename TIter, typename TFunc,
	typename TMappedIter = mapped_iterator<TIter, TFunc> >
static auto make_mapped_range(TIter begin, TIter end, TFunc func) -> iterator_range<TMappedIter>
{ return iterator_range<TMappedIter>(make_mapped_iterator(begin, func), make_mapped_iterator(end, func)); }

#endif

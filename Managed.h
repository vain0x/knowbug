// Managed Value (of hsp) by reference counting

/**
class Managed<T>

参照数方式のスマートポインタの一種。
「バッファを自前で確保する」使い方と、既にある T への弱参照としての使い方を両方できる。
また、参照カウントに加えて「一時オブジェクトフラグ」を持つ。HSPのスタックに積まれるオブジェクトには、必ずこれを立てる。

nullptr に加えて、MagicNull という無効値を持つ。
これは、Managed<> を実体とする変数型を使う際に、その比較演算の結果として HspBool を書き込む必要があるため。

追加のテンプレート引数で、default ctor における動作を設定できる。もっとうまい方法がよい。

なお Managed<TBase> ←→ Managed<TDerived> のアップキャスト、ダウンキャストはできない。不便。
また、Managed<T> に TDerived* を所有させるなら、T の destructor (dtor) が virtual であるか、
TDerived とその基底クラスのすべての dtor が trivial でなければならない。(Managed<> が型消去を使っていないため。)

(sizeof(Managed<T>) == sizeof(void*)) という制約がある。
vector_k 型は PVal::master の領域に Managed<> を配置 new する。

todo: 学ぶ
memo: header の中にカスタムデリーターを入れておくことも可能

//*/

#ifndef IG_MANAGED_H
#define IG_MANAGED_H

#include <memory>

#include "hsp3plugin_custom.h"
#include "HspAllocator.h"

#define DBGOUT_MANAGED_REFCNT    FALSE//TRUE
#define DBGOUT_MANAGED_KILLED    FALSE//TRUE
#define DEBUG_MANAGED_USING_INSTANCE_ID (DBGOUT_MANAGED_REFCNT || DBGOUT_MANAGED_KILLED)
#if DEBUG_MANAGED_USING_INSTANCE_ID
static unsigned char newManagedInstanceId() { static unsigned char id_; return id_++; }
#endif

namespace hpimod
{

template<typename TValue,
	// inst_ を nullptr で初期化するか否か
	bool bNullCtor,

	// for construct, destroy
	typename Allocator = HspAllocator<TValue>
>
class Managed {
	using value_type = TValue;
	using self_t = Managed<value_type, bNullCtor, Allocator>;
	
	// desiable: sizeof(inst_t with value_type) <= 64, because of specs of HspAllocator.
	struct inst_t {
		int paddings_[2];
		mutable int cnt_;
		mutable bool tmpobj_;
		unsigned char padding_;		// (for instance id while debugging)
		unsigned short const magicCode_;	//= MagicCode

		// flexible structure (used as T value)
		char value_[1];
	};
	static size_t const instHeaderSize = 3*sizeof(int)+sizeof(bool)+sizeof(unsigned char)+sizeof(unsigned short);
	static unsigned short const MagicCode = 0x55AB;

	inst_t* inst_;

public:
	// null ctor by force
	// Must check null to use
	explicit Managed(nullptr_t) : inst_ { nullptr } {
		// static_assert
		assert(sizeof(self_t) == sizeof(void*));
	}
private:
	using CharAllocator = HspAllocator<char>;

	template<typename TAllocator = Allocator>
	static TAllocator& getAllocator()
	{
		static_assert(std::is_empty<TAllocator>::value, "Managed<> can use only stateless alloctor.");
		static TAllocator stt_allocator {};
		return stt_allocator;
	}
	
	template<typename TInit>
	void initializeHeader()
	{
		assert(!inst_);
		inst_ = reinterpret_cast<inst_t*>(
			std::allocator_traits<CharAllocator>::allocate(getAllocator<CharAllocator>(), instHeaderSize + sizeof(TInit))
		);
		new(inst_)inst_t { {}, 1, false, '\0', MagicCode, {} };
		
		assert(static_cast<void const*>(&inst_->magicCode_ + 1) == (inst_->value_)
			&& isManagedValue(reinterpret_cast<value_type*>(inst_->value_)));

		// new(inst_->value_) TInit(...);
#if DEBUG_MANAGED_USING_INSTANCE_ID
		inst_->padding_ = newManagedInstanceId();
	}
	int instId() const { assert(!isNull()); return inst_->padding_; }
#else
	}
#endif

public:
	// default ctor
	Managed() : inst_ { nullptr } { defaultCtor(); }

private:
	// overload by SFINAE
	template<typename TVoid = void,
		std::enable_if_t<bNullCtor, TVoid>* = nullptr
	> void defaultCtor()
	{ }

	template<
		typename TVoid = void,
		std::enable_if_t<!bNullCtor, TVoid>* = nullptr
	> void defaultCtor()
	{
		initializeHeader<value_type>();
		std::allocator_traits<Allocator>::construct(getAllocator(), valuePtr());
	}

public:
	// 実体の生成を伴う factory 関数
	template<typename TDerived = value_type, typename ...Args>
	static self_t makeDerived(Args&&... args)
	{
		static_assert(std::is_class<TDerived>::value && std::is_convertible<TDerived*, value_type*>::value, "互換性のない型では初期化できない。");
		static_assert(std::is_same<value_type, TDerived>::value || std::has_trivial_destructor<TDerived>::value || std::has_virtual_destructor<value_type>::value,
			"Managed<T> が T の派生型を所有するためには、その派生型が trivial destructor を持つか、T が virtual な destructor を持たなければならない。正常に解放できないため。");
		static_assert(std::is_constructible<TDerived, Args...>::value, "constructor TDerived(Args...) is not found.");

		self_t self { nullptr }; self.initializeHeader<TDerived>();

		using rebound_t = std::allocator_traits<Allocator>::rebind_alloc<TDerived>;
		std::allocator_traits<rebound_t>::construct(getAllocator<rebound_t>(),
			reinterpret_cast<TDerived*>(self.valuePtr()),
			std::forward<Args>(args)...
		);
		return std::move(self);
	}

	template<typename ...Args>
	static self_t make(Args&&... args)
	{
		return makeDerived<value_type>(std::forward<Args>(args)...);
	}

	// copy
	Managed(self_t const& rhs)
		: inst_ { rhs.inst_ }
	{ incRef(); }

	self_t& operator=(self_t rhs) { swap(rhs); return *this; }

	// move
	Managed(self_t&& rhs)
		: inst_ { rhs.inst_ }
	{ rhs.inst_ = nullptr; }

	// swap
	void swap(self_t& rhs) throw() { std::swap(inst_, rhs.inst_); }

public:
	// 実体ポインタから managed を作成する factory 関数 (failure: nullptr)
	// inst_t::value_ を指しているはずなので、inst_t の先頭を逆算する。
	static self_t const ofValptr(value_type const* pdat) {
		auto const inst = reinterpret_cast<inst_t const*>(reinterpret_cast<char const*>(pdat) - instHeaderSize);
	//	assert(inst->magicCode_ == MagicCode);
		return self_t { const_cast<inst_t*>(inst) };
	};
	static self_t ofValptr(value_type* pdat) { return const_cast<self_t&&>(ofValptr(static_cast<value_type const*>(pdat))); }

private:
	explicit Managed(inst_t* inst) : inst_ { inst }
	{ incRef(); }

public:
	~Managed() {
		decRef();
	}

	// 初期状態に戻す
	void reset() { swap(self_t {}); }

	// nullptr にクリアする
	// bNullCtor に依らず nullptr になるので注意して使うこと。
	void nullify() { swap(self_t { nullptr }); }

private:
	void kill() const {
		assert(isManaged() && cnt() == 0);

#if DBGOUT_MANAGED_KILLED
		dbgout("[%d] KILL %d <%d>", instId(), cnt(), tmpobj());
#endif
		std::allocator_traits<Allocator>::destroy(getAllocator(), valuePtr());
		std::allocator_traits<CharAllocator>::deallocate(getAllocator<CharAllocator>(),
			reinterpret_cast<char*>(inst_), 1);		// HspAllocator<> ignores counts to deallocate.
	}

private:
	// アクセサ
	int& cnt() const { assert(!isNull()); return reinterpret_cast<int&>(inst_->cnt_); }
	bool& tmpobj() const { assert(!isNull()); return reinterpret_cast<bool&>(inst_->tmpobj_); }

public:
	int cntRefers() const { return cnt(); }
	bool isTmpObj() const { return tmpobj(); }

	value_type* valuePtr() const { assert(!isNull()); return reinterpret_cast<value_type*>(inst_->value_); }
	value_type& value() const { return *valuePtr(); }

private:
	// 参照カウンタとしての機能
#if DBGOUT_MANAGED_REFCNT
	void incRefImpl() const { assert( isManaged() );
		dbgout("[%d] inc(++) %d -> %d <%d>", instId(), cnt(), cnt() + 1, tmpobj());
		++cnt(); }
	void decRefImpl() const { assert( isManaged() );
		dbgout("[%d] dec(--) %d -> %d <%d>", instId(), cnt(), cnt() - 1, tmpobj());
		--cnt(); if ( cnt() == 0 ) { kill(); } }
#else
	void incRefImpl() const { assert( isManaged() ); ++cnt(); }
	void decRefImpl() const { assert( isManaged() ); --cnt(); if ( cnt() == 0 ) { kill(); } }
#endif

public:
	void incRef() const { if ( isManaged() ) incRefImpl(); }
	void decRef() const { if ( isManaged() ) decRefImpl(); }

	self_t& beTmpObj() {	// const でもいいかも
		if ( isManaged() ) {
			assert(!isTmpObj());
			tmpobj() = true; incRefImpl();
		}
		return *this;
	}
	void beNonTmpObj() const {
		if ( isManaged() && isTmpObj() ) {
			tmpobj() = false; decRefImpl();
		}
	}

public:
	// その他
	bool isNull() const {
		int const i = reinterpret_cast<int>(inst_);
		return (i == HspTrue || i == HspFalse);
	}
	bool isManaged() const {
		return (!isNull() && inst_->magicCode_ == MagicCode);
	}

	// data が構造体 Managed<T>::inst_t の中の value_ を指しているか否か
	static bool isManagedValue(value_type const* data) {
		return (reinterpret_cast<unsigned short const*>(data)[-1] == MagicCode);
	}

public:
	// 演算子
	value_type& operator*() const { return value(); }
	value_type* operator->() const { return valuePtr(); }

	bool operator==(self_t const& rhs) const {
		return (isNull() && rhs.isNull()) || (inst_ == rhs.inst_);
	}
	bool operator!=(self_t const& rhs) const { return !(*this == rhs); }
};

} // namespace hpimod

#endif

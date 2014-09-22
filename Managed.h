// Managed Value (of hsp) by reference counting

/**
class Managed<T>

参照カウンタ方式のスマートポインタの一種。
「バッファを自前で確保する」使い方と、既にある T への弱参照としての使い方を両方できる。
また、参照カウントに加えて「一時オブジェクトフラグ」を持つ。HSPのスタックに積まれるオブジェクトには、必ずこれを立てる。

nullptr に加えて、MagicNull という無効値を持つ。
これは、Managed<> を実体とする変数型を使う際に、その比較演算の結果として HspBool を書き込む必要があるため。

追加のテンプレート引数で、default ctor における動作を設定できる。もっとうまい方法がよい。

なお Managed<TDerived> → Managed<TBase> のアップキャストはできない。実に不便。
また、Managed<T> に TDerived* を所有させるなら、T の destructor (dtor) が virtual であるか、
TDerived とその基底クラスのすべての dtor が trivial でなければならない。(後者は制約としてコードされていない。)

(sizeof(Managed<T>) == sizeof(void*)) という制約がある。
vector_k 型は PVal::master の領域に Managed<> を配置 new する。

todo: 学ぶ

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
static unsigned char newManagedInstanceId() { static unsigned char id_ { 0 }; return id_++; }
#endif

namespace hpimod {

namespace detail
{
template<typename T>
struct DefaultCtorDtor {
	static inline void defaultCtor(T* p) { new(p) T(); }
	static inline void defaultDtor(T& self) { self.~T(); }
};
}

template<typename TValue,
	// inst_ を nullptr で初期化するかどうか
	bool bNullCtor,
	typename DefaultCtorDtor = detail::DefaultCtorDtor<TValue>
>
class Managed {
	struct inst_t {
		mutable int cnt_;
		mutable bool tmpobj_;
		unsigned char padding_;		// (for instance id while debugging)
		unsigned short const magicCode_;	//= MagicCode

		// flexible structure (used as T object)
		char value_[1];
	};

	inst_t* inst_;

private:
	using value_type = TValue;
	using self_t = Managed<value_type, bNullCtor, DefaultCtorDtor>;
	
	static size_t const instHeaderSize = sizeof(int)+sizeof(bool)+sizeof(unsigned char)+sizeof(unsigned short);

	static unsigned short const MagicCode = 0x55AB;

private:
	template<typename TInit>
	void initializeHeader()
	{
		assert(!inst_ && exinfo && hspmalloc);
		inst_ = reinterpret_cast<inst_t*>(hspmalloc(instHeaderSize + sizeof(TInit)));
		::new(inst_)inst_t { 1, false, '\0', MagicCode, {} };
		assert(inst_->cnt_ == 1 && inst_->tmpobj_ == false && inst_->magicCode_ == MagicCode
			&& static_cast<void const*>(&inst_->magicCode_ + 1) == (inst_->value_) && isManagedValue(reinterpret_cast<value_type*>(inst_->value_)));
		// new(inst_->value_) TInit(...);
#if DEBUG_MANAGED_USING_INSTANCE_ID
		inst_->padding_ = newManagedInstanceId();
	}
	int instId() const { return inst_->padding_; }
#else
	}
#endif

public:
	// default ctor
	Managed() : inst_ { nullptr } {
		if ( !bNullCtor ) {
			initializeHeader<value_type>();
			DefaultCtorDtor::defaultCtor(valuePtr());
		}
	}

	// 実体の生成を伴う factory 関数
	template<typename TDerived = value_type, typename ...Args>
	static self_t makeDerived(Args&&... args)
	{
		static_assert(std::is_convertible<TDerived*, value_type*>::value, "互換性のない型では初期化できない。");
		static_assert(std::is_same<value_type, TDerived>::value || std::has_virtual_destructor<value_type>::value,
			"Managed<T> は、T が virtual destructor を持たないかぎり、T の派生型を所有できない。正常に解放できないため。");

		self_t self { nullptr }; self.initializeHeader<TDerived>();
		new(self.valuePtr()) TDerived(std::forward<Args>(args)...);
		return std::move(self);
	}

	// 実体の生成を伴う factory 関数
	template<typename ...Args>
	static self_t make(Args&&... args)
	{
		return makeDerived<value_type>(std::forward<Args>(args)...);
	}

	explicit Managed(nullptr_t) : inst_ { nullptr } {
		// static_assert
		assert(sizeof(self_t) == sizeof(void*));
	}

	// copy
	Managed(self_t const& rhs)
		: inst_ { rhs.inst_ }
	{ incRef(); }

	// move
	Managed(self_t&& rhs)
		: inst_ { rhs.inst_ }
	{ rhs.inst_ = nullptr; }

#if 0
	// 値渡しで初期化する factory 関数
	static self_t ofValue(value_type const& src) { return make(src); }
	static self_t ofValue(value_type&& src) { return make(std::move(src)); }
#endif

public:
	// instptr から managed を作成する factory 関数
	static self_t const ofInstptr(void const* inst) { return self_t { const_cast<inst_t*>(static_cast<inst_t const*>(inst)) }; };
	static self_t ofInstptr(void* inst) { return const_cast<self_t&&>(ofInstptr(static_cast<void const*>(inst))); }

private:
	explicit Managed(inst_t* inst) : inst_ { inst }
	{
	//	assert(checkMagicCode());
		incRef();
	}

public:
	// 実体ポインタから managed を作成する factory 関数 (failure: nullptr)
	// inst_t::value_ を指しているはずなので、inst_t の先頭を逆算する。
	static self_t const ofValptr(value_type const* pdat) {
		auto const inst = reinterpret_cast<inst_t const*>(reinterpret_cast<char const*>(pdat) - instHeaderSize);
	//	assert(inst->magicCode_ == MagicCode);
		return ofInstptr(inst);
	};
	static self_t ofValptr(value_type* pdat) { return const_cast<self_t&&>(ofValptr(static_cast<value_type const*>(pdat))); }

public:
	// デストラクタ
	~Managed() {
		decRef();
	}

	// 初期状態に戻す
	void reset() {
		this->~Managed(); new(this) self_t {};
	}

	// nullptr にクリアする
	// bNullCtor に依らず nullptr になるので注意して使うこと。
	void nullify() { decRef(); inst_ = nullptr; }

private:
	void kill() const {
		assert(!isNull() && cnt() == 0);
		auto* const inst_bak = inst_;
		auto& value_bak = value();

#if DBGOUT_MANAGED_KILLED
		dbgout("[%d] KILL %d <%d>", instId(), cnt(), tmpobj());
#endif
		// このオブジェクトからの参照を切り、MagicCode を消しておく (値の解体中に this が参照される際の安全のため)
		const_cast<unsigned short&>(inst_->magicCode_) = 0;
		const_cast<inst_t*&>(inst_) = nullptr;

		DefaultCtorDtor::defaultDtor(value_bak);
		hspfree(inst_bak);
	}

private:
	// アクセサ
	int& cnt() const { return reinterpret_cast<int&>(inst_->cnt_); }
	bool& tmpobj() const { return reinterpret_cast<bool&>(inst_->tmpobj_); }

public:
	int cntRefers() const { return cnt(); }
	bool isTmpObj() const { return tmpobj(); }

	value_type* valuePtr() const { return reinterpret_cast<value_type*>(inst_->value_); }
	value_type& value() const { return *valuePtr(); }

	// inst のポインタを返す
	void* instPtr() const { return inst_; }

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
	self_t& operator=(self_t const& rhs) { this->~Managed(); new(this) Managed(rhs); return *this; }
	self_t& operator=(self_t&& rhs) { this->~Managed(); new(this) Managed(std::move(rhs)); return *this; }

	value_type& operator*() const { return value(); }
	value_type* operator->() const { return valuePtr(); }

	bool operator==(self_t const& rhs) const {
		return (isNull() && rhs.isNull()) || (inst_ == rhs.inst_);
	}
	bool operator!=(self_t const& rhs) const { return !(*this == rhs); }
};

} // namespace hpimod

#endif

// Managed Value (of hsp) by reference counting

// std::shared_ptr<> に似ているが、nullptr の他に MagicNull という無効値を持つことが異なる。

// 派生品
// assoc (<CAssoc>), vector (<CVector>)

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

// sizeof(Managed<T>) == sizeof(void*)
// Managed でない T も一緒に扱える。
// vector_k: pval->master の領域をこのインスタンスとして使う。
//	また、vector の各要素として ManagedPVal を使う。

namespace detail
{
template<typename T>
struct DefaultCtorDtor {
	static inline void defaultCtor(T* p) { new(p) T(); }
	static inline void defaultDtor(T& self) { self.~T(); }
};
}

template<typename T,
	// 付随値の型
	typename TPalam,
	// inst_ を nullptr で初期化するかどうか
	bool bNullCtor,
	typename DefaultCtorDtor = detail::DefaultCtorDtor<T>
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
	using value_type = T;
	using self_t = Managed<T, bNullCtor, DefaultCtorDtor>;

	static size_t const valueSize = sizeof(T);
	static size_t const instHeaderSize = sizeof(int)+sizeof(bool)+sizeof(unsigned char)+sizeof(unsigned short);
	static size_t const instSize = valueSize + instHeaderSize;

	static unsigned short const MagicCode = 0x55AB;

private:
	void initializeHeader()
	{
		assert(!inst_ && exinfo && hspmalloc);
		inst_ = reinterpret_cast<inst_t*>(hspmalloc(instSize));
		::new(inst_)inst_t { {}, 1, false, '\0', MagicCode, {} };
		assert(inst_->cnt_ == 1 && inst_->tmpobj_ == false && inst_->magicCode_ == MagicCode
			&& static_cast<void const*>(1 + &inst_->magicCode_) == (inst_->value_));
		// new(inst_->value_) T(...);
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
			initializeHeader();
			DefaultCtorDtor::defaultCtor(valuePtr());
		}
	}

	// 実体の生成を伴う factory 関数
	template<typename ...Args>
	static self_t make(Args&&... args)
	{
		self_t self { nullptr }; self.initializeHeader();
		new(self.valuePtr()) T(std::forward<Args>(args)...);
		return std::move(self);
	}

	explicit Managed(nullptr_t) : inst_ { nullptr } {
		// static_assert
		assert(instSize == 0 || instSize % 2 == 0);
		assert(sizeof(self_t) == 4);
	}

	// copy
	Managed(self_t const& rhs)
		: inst_ { rhs.inst_ }
	{ incRef(); }

	// move
	Managed(self_t&& rhs)
		: inst_ { rhs.inst_ }
	{ rhs.inst_ = nullptr; }

	// 値渡しで初期化する factory 関数
	static self_t ofValue(T&& src)
	{
		return make(std::forward<T>(src));
	}

public:
	// instptr から managed を作成する factory 関数
	static self_t const ofInstptr(void const* inst) { return self_t { static_cast<inst_t*>(inst) } };
	static self_t ofInstptr(void* inst) { return const_cast<self_t&&>(ofInstptr(static_cast<void const*>(inst))); }

private:
	explicit Managed(inst_t* inst) : inst_ { inst }
	{
		assert(checkMagicCode());
		incRef();
	}

	// 実体ポインタから managed を作成する factory 関数 (failure: nullptr)
	// inst_t::value_ を指しているはずなので、inst_t の先頭を逆算する。
	static self_t const ofValptr(void const* pdat) {
		auto const inst = reinterpret_cast<inst_t*>(reinterpret_cast<char*>(pdat) - instHeaderSize);
		asssert(inst->magicCode_ == MagicCode);
		return ofInstptr(inst);
	};
	static self_t ofValptr(void* pdat) { return const_cast<self_t&&>(ofValptr(static_cast<void const*>(pdat))); }

public:
	// デストラクタ
	~Managed() {
		decRef();
	}

	// 初期状態に戻す
	void clear() {
		this->~Managed(); new(this) self_t {};
	}

	// nullptr にクリアする
	void clear(nullptr_t) { decRef(); inst_ = nullptr; }

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

	T* valuePtr() const { return reinterpret_cast<T*>(inst_->value_); }
	T& value() const { return *valuePtr(); }

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
	//static bool isManagedValue(T* data) {
	//	return (*reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(data) - sizeof(unsigned short)) == MagicCode);
	//}

public:
	// 演算子
	self_t& operator=(self_t const& rhs) { this->~Managed(); new(this) Managed(rhs); return *this; }
	self_t& operator=(self_t&& rhs) { this->~Managed(); new(this) Managed(std::move(rhs)); return *this; }

	T& operator*() const { return value(); }
	T* operator->() const { return &value(); }

	bool operator==(self_t const& rhs) const {
		return (isNull() && rhs.isNull()) || (inst_ == rhs.inst_);
	}
	bool operator!=(self_t const& rhs) const { return !(*this == rhs); }
};

} // namespace hpimod

#endif

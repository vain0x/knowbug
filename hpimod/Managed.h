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
Managed<T> に TDerived* を所有させるには、T の destructor が virtual か trivial でなければならない。(Managed<> が型消去を使っていないため。)

todo: alignment の問題に対処
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
	//to construct with nullptr by default; or construct with default ctor
	bool bNullCtor,

	//for construction and destruction
	//buffer is allocated by using HspAllocator
	typename Allocator = HspAllocator<TValue>
>
class Managed {
	using value_type = TValue;
	using self_t = Managed<value_type, bNullCtor, Allocator>;
	using byte = unsigned char;
	
	struct Inst {
		int paddings_[2];
		mutable int cnt_;
		mutable bool tmpobj_;
		byte padding_; // (for instance id while debugging)
		unsigned short magicCode_; //= MagicCode

		// flexible structure (used as T value)
		byte value_[1];
	};
	static size_t const instHeaderSize = sizeof(Inst) - sizeof(byte[1]);
	static unsigned short const MagicCode = 0x55AB;

	Inst* inst_;

public:
	// null ctor by force
	// Must check null to use
	explicit Managed(nullptr_t) : inst_ { nullptr } {
		static_assert(sizeof(self_t) == sizeof(void*), "Managed<> must have the same size with a data pointer");
	}
private:
	using ByteAllocator = HspAllocator<byte>;

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
		static size_t const Size = instHeaderSize + sizeof(TInit);
		static_assert(Size <= 64, "Manage<> should be small.");
		inst_ = reinterpret_cast<Inst*>(
			std::allocator_traits<ByteAllocator>::allocate(getAllocator<ByteAllocator>(), Size)
		);
		inst_->cnt_ = 1;
		inst_->tmpobj_ = false;
		inst_->magicCode_ = MagicCode;

		assert(static_cast<void const*>(&inst_->magicCode_ + 1) == (inst_->value_)
			&& isManagedValue(reinterpret_cast<value_type*>(inst_->value_)));

		// new(inst_->value_) TInit(...);
#if DEBUG_MANAGED_USING_INSTANCE_ID
		inst_->padding_ = newManagedInstanceId();
#endif
	}
#if DEBUG_MANAGED_USING_INSTANCE_ID
	int instId() const { assert(!isNull()); return inst_->padding_; }
#endif

public:
	// default ctor
	Managed() : inst_ { nullptr } { defaultCtor<bNullCtor>(); }

private:
	template<bool bNullCtor = true> void defaultCtor()
	{ }
	template<> void defaultCtor<false>()
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
		auto derivedPtr = reinterpret_cast<TDerived*>(self.valuePtr());

		using rebound_t = std::allocator_traits<Allocator>::rebind_alloc<TDerived>;
		std::allocator_traits<rebound_t>::construct(getAllocator<rebound_t>(),
			derivedPtr, std::forward<Args>(args)...
		);

		// キャストでアドレスが変わる型ではいけない
		assert( static_cast<value_type*>(derivedPtr) == self.valuePtr() );

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

	void swap(self_t& rhs) throw() { using std::swap; swap(inst_, rhs.inst_); }

public:
	//factory to make managed from value pointer (which should point to Inst::value_)
	//returns nullptr if failed
	static self_t const ofValptr(value_type const* pdat)
	{
		if ( !pdat ) return self_t { nullptr };

		auto const inst = reinterpret_cast<Inst const*>(reinterpret_cast<byte const*>(pdat) - instHeaderSize);
		//assert(inst->magicCode_ == MagicCode);
		return self_t { const_cast<Inst*>(inst) };
	};
	static self_t ofValptr(value_type* pdat) { return const_cast<self_t&&>(ofValptr(static_cast<value_type const*>(pdat))); }

private:
	explicit Managed(Inst* inst) : inst_ { inst }
	{ incRef(); }

public:
	~Managed() {
		decRef();
	}

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
		std::allocator_traits<ByteAllocator>::deallocate(getAllocator<ByteAllocator>(),
			reinterpret_cast<byte*>(inst_), 1); // HspAllocator<> ignores counts to deallocate.
	}

private:
	int& cnt() const { assert(!isNull()); return reinterpret_cast<int&>(inst_->cnt_); }
	bool& tmpobj() const { assert(!isNull()); return reinterpret_cast<bool&>(inst_->tmpobj_); }

public:
	int cntRefers() const { return cnt(); }
	bool isTmpObj() const { return tmpobj(); }

	value_type* valuePtr() const { assert(!isNull()); return reinterpret_cast<value_type*>(inst_->value_); }
	value_type& value() const { return *valuePtr(); }

private:
	//reference count part
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

	self_t& beTmpObj() {
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
	bool isNull() const {
		int const i = reinterpret_cast<int>(inst_);
		return (i == HspTrue || i == HspFalse);
	}
	bool isManaged() const {
		return (!isNull() && inst_->magicCode_ == MagicCode);
	}

	// data が構造体 Managed<T>::Inst の中の value_ を指しているか否か
	static bool isManagedValue(value_type const* data) {
		return (reinterpret_cast<unsigned short const*>(data)[-1] == MagicCode);
	}

public:
	value_type& operator*() const { return value(); }
	value_type* operator->() const { return valuePtr(); }

	explicit operator bool() const { return !isNull(); };

	bool operator==(self_t const& rhs) const {
		return (isNull() && rhs.isNull()) || (inst_ == rhs.inst_);
	}
	bool operator!=(self_t const& rhs) const { return !(*this == rhs); }
};

} // namespace hpimod

#endif

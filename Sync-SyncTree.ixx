export module Sync : SyncTree;
import : Abstractions;
import : LockContext;
import : SemaContext;
import std;
using namespace std;

constinit static nullptr_t n = nullptr;

// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //

/*\
 * SyncTree represents a synchronization sequence as a recurseive tree structure
 * with three branches:
 * - Left branch: Locking sequence
 * - Middle branch: Synchronized context
 * - Right branch: Unlocking sequence
 * A SyncTree composed of only a root node is equivalent to a critical section
 * protected by a single lock.
 * 
 * When a SyncTree structure is invoked with a delegated action, it traverses its
 * left side in order to enter its synchronized context before invoking the
 * delegated action; afterward, it similarly traverses its right side to exit
 * the synchronized context.
 * 
 * The left and right branches of any SyncTree node treat mutex::lock and
 * mutex::unlock as their own delegated actions, except when the branches are
 * absent, in which case they are invoked immediately. Since a SyncTree node and
 * its middle child can be processed the same way, an invocation will traverse
 * the entire chain of nodes composing the middle branch in a loop rather than
 * recursively invoking each one.
\*/

// SyncTree's template parameter must satisfy IsContext for the concrete
// implementations, or be void for the abstract base class.
template<typename T>
concept STType = IsContext<T> || IsVoid<T>;

export template<STType C = LC<void>> class SyncTree;
template<STType C> using ST = SyncTree<C>;

template<IsBits> class DelegateContext;



using SP = ST<void> const * const;
using NavMember = SP ST<void>::*;

template<typename... P, Inv<P...> F>
auto CallImpl(ST<void> const &, F&&, P&&...) -> Ret<F, P...>;

template<IsContext C>
auto NaviImpl(ST<C> const &, NavMember) -> void;

// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //

template<IsContext C, typename... T> SyncTree(C, T...) -> SyncTree<C>;

template<> class SyncTree<void> {
private:
	template<STType> friend class SyncTree;
	template<typename... P, Inv<P...> F>
	friend auto CallImpl(SyncTree<void> const &, F&&, P&&...) -> Ret<F, P...>;
	template<IsContext C>
	friend auto NaviImpl(SyncTree<C> const &, NavMember) -> void;
protected:
	SP L, M, R;

	virtual auto Navigate(NavMember) const -> void = 0;
	virtual auto Trigger(NavMember) const -> void = 0;
public:
	SyncTree(      SP m      ) : SyncTree<void>{ n,m,n } {}
	SyncTree(SP l,       SP r) : SyncTree<void>{ l,n,r } {}
	SyncTree(SP l, SP m, SP r) : L{ l }, M{ m }, R{ r } {}

	~SyncTree() { delete L; delete M; delete R; }

	template<typename... P, Inv<P...> F>
	auto operator() (F&& f, P&&... p) const -> Ret<F, P...>
	{ return CallImpl(*this, forward<F>(f), forward<P>(p) ... ); }
};

template<IsContext Ctx> class SyncTree<Ctx> : public SyncTree<void> {
protected:
	template<STType> friend class SyncTree; // Needed in Send
	Ctx& C;
public:
	SyncTree(Ctx& c                  ) : C{ c }, SyncTree<void>{ n,n,n } {}
	SyncTree(Ctx& c,       SP m      ) : C{ c }, SyncTree<void>{ n,m,n } {}
	SyncTree(Ctx& c, SP l,       SP r) : C{ c }, SyncTree<void>{ l,n,r } {}
	SyncTree(Ctx& c, SP l, SP m, SP r) : C{ c }, SyncTree<void>{ l,m,r } {}
	
	using SyncTree<void>::SyncTree; // Inherit constructors
	//using NavAction = auto (Ctx::*)() -> void; // lock or unlock
	auto Navigate(NavMember side) const -> void { NaviImpl(*this, side); };
	auto Trigger(NavMember side) const -> void {
		(C.*(side == &SyncTree<void>::L ? &Ctx::lock : &Ctx::unlock))();
	}

	// TODO: Should this be a pure virtual function on the abstract base class?
	template<IsBits O> auto Send(unsigned = 0) -> DelegateContext<O> const;
};

// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //

template<IsContext Ctx>
template<IsBits O>
auto SyncTree<Ctx>::Send(unsigned idx) -> DelegateContext<O> const {
	static type_info const & LCType{ typeid(SyncTree<LC<O>>) };
	static type_info const & SCType{ typeid(SyncTree<SC<O>>) };

	remove_const_t<SP> Node{ this };

	// 1: If types match, decrement index counter
	//    1A: If index counter reaches 0, break out
	// 2: Hard stop when node advances to nullptr
	for (unsigned i{ idx + 1 }; Node; Node = Node->M)
		if(typeid(*Node) == LCType && !--i)
			return DelegateContext<O>{ (O&)(((SyncTree<LC<O>>*)Node)->C), this };
		else
		if(typeid(*Node) == SCType && !--i)
			return DelegateContext<O>{ (O&)(((SyncTree<SC<O>>*)Node)->C), this };

	throw invalid_argument
		("Couldn't find "s + typeid(O).name() + ' ' + to_string(idx));

	// Cast to LockContext<O>, the actual verified type (or base), so we can
	// properly cast-expose the protected Data of interest.
	// - Casting directly from LockContext<void> produces junk data.
	// - Casting to O& is necessary because passing the argument into a
	//   template function that uses CTAD will make it resolve to
	//   LockContext<O> instead.
	// SemaContext inherits from LockContext, and so can be cast to it without
	// issue when exposing the protected Data.
}

// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //

// SyncTree<T> can be used in two ways:
// 1: Direct invocation using operator() that treats it as a simple locking
//    structure, passing none of its protected data into the synchronized context
// 2: Send<U>(unsigned N) function template, requires IsBits<U>:
//    - Returns a DelegateContext<U> with a member reference to the U protected
//      by the LockContext<U> or SemaContext<U> referenced by the Nth
//      SyncTree<<U>> encountered from SyncTree<T> where Send was called.
//    - DelegateContext<U>'s To(...) function can be invoked with any function
//      that accepts a U& as its first parameter. If the function has more
//      parameters, they must be supplied as additional arguments to To in the
//      same order as the passed function expects them. To will perfectly forward
//      them to the function.
// Whether SyncTree<T>::operator() or SyncTree<T>::Send<>() is used, both operate
// in the same synchronized context.
template<IsBits O> class DelegateContext {
private:
	template<STType> friend class SyncTree;
	O const & Item;
	SyncTree<void> const * const Entrance;
	DelegateContext(O& i, SyncTree<void> const * const e)
	: Item{ i }, Entrance{ e } {}
public:
	template<typename... P, Inv<O&, P...> F>
	auto To(F&& f, P&&... p) const && -> Ret<F, O&, P...>
	{ CallImpl(*Entrance, forward<F>(f), Item, forward<P>(p) ... ); }
};

// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //
// --------------------------------------------------------------------------- //

// TODO: Incorporate as member functions when deducing this becomes available in
// the compiler.

template<typename... P, Inv<P...> F>
auto CallImpl(SyncTree<void> const & st, F&& f, P&&... p) -> Ret<F, P...> {
	using R = Ret<F, P...>;
	st.Navigate(&SyncTree<void>::L);
	if constexpr (IsVoid<R>) {
		invoke(forward<F>(f), forward<P>(p) ... );
		return st.Navigate(&SyncTree<void>::R);
	} else {
		R r{ invoke(forward<F>(f), forward<P>(p) ... ) };
		return st.Navigate(&SyncTree<void>::R), r;
	}
}

template<IsContext C>
auto NaviImpl(SyncTree<C> const & st, NavMember side) -> void {
	for(remove_const_t<SP> Node{ &st }; Node; Node = Node->M) {
		SP & Side { Node->*side };
		
		if(Side) (Side)->Navigate(&SyncTree<void>::L);
		Node->Trigger(side); // Trigger = Pure virtual
		if(Side) (Side)->Navigate(&SyncTree<void>::R);
	}
}
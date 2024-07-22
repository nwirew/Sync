module;

// std::forward shorthand
#define fw(x) forward<decltype(x)>(x)

// Verify that invocable entity F can be invoked with parameters P...
#define CanInvoke(f, p) Inv<decltype(f), decltype(p) ... >

// Call std::invoke with invocable entity F and parameters P
#define Delegate(f, p) invoke(fw(f), fw(p) ... )

// LockContext member functions all exhibit the same form save for a few
// different actions between invoking the delegated function and returning, and
// those actions in turn are duplicated across if-constexpr compilation paths.
// This macro consolidates the shared implementation details into a single point
// of contact.
#define DelegateWithContext(Fun, Ctx, Pre, Post) \
template<typename T> \
auto Fun(LC<T> & Ctx, auto && f, auto &&... p) requires CanInvoke(f, p) { \
	cout << "+++++ Function called in " << typeid(Ctx).name() << endl; \
	using R = Ret<decltype(f), decltype(p)...>; \
	Pre; \
	if constexpr (IsBits<R>) { \
		R r{ Delegate(f, p) }; \
		return Post, r; \
	} else { \
		Delegate(f, p); \
		return Post; \
	} \
}

export module Sync : LockContext;
import : Abstractions;
import std;
using namespace std;

/*\
 * LockContext combines std::mutex and std::condition_variable_any into a single
 * class, wrapping certain inherited features with simpler interfaces while also
 * providing additional methods of protection and synchronization. Additionally,
\*/

// --------------------------------------------------------------------------- //
// --- Notes ----------------------------------------------------------------- //
// --------------------------------------------------------------------------- //

// TODO: This might be a prime use case for deducing this:
// https://devblogs.microsoft.com/cppblog/cpp23-deducing-this/#crtp
// 
// But it needs more compiler support first!
// 
// Changes it should allow:
// - Move code from MarcoImpl and PoloImpl to LockContext<void>::Marco and
//   LockContext<void>::Polo
// - Remove LockContext<T>::Marco and LockContext<T>::Polo - LockContext<T> will
//   use the versions inherited from LockContext<void>
// - operator() could also be consolidated but then would need extra testing for
//   what arguments to pass

// --------------------------------------------------------------------------- //
// --- Exports and forward declarations -------------------------------------- //
// --------------------------------------------------------------------------- //

export template<typename> class LockContext;
export template<typename T> using LC = LockContext<T>;

template<typename T>
LockContext(T&&) -> LC<T>;
LockContext(   ) -> LC<void>;

// All LC and SC either are or inherit from LC<void>
export template<typename C>
concept IsContext = Inherits<C, LC<void>>;

using CVA = condition_variable_any;
using MTX = mutex;

// I justify these function names being single-character by them also being
// local to this module
DelegateWithContext( M, c, (c.lock()), (c.wait(), c.unlock()) )
DelegateWithContext( P, c, (c.lock()), (c.unlock(), c.notify_one()) )
DelegateWithContext( C, c, (c.lock()), (c.unlock()) )

// --------------------------------------------------------------------------- //
// --- LockContext class templates ------------------------------------------- //
// --------------------------------------------------------------------------- //

template<IsVoid DataType> class LockContext<DataType> : public CVA, public MTX {
public:
	auto wait(auto && p) -> void requires Is<Ret<decltype(p)>, bool>
	{ CVA::wait(*this, p); }
	auto wait() -> void
	{ CVA::wait(*this); }

	// Marco Polo: Check notes near end of file
	auto Marco(auto && f, auto && ... p)
	{ return M(*this, fw(f), *this, fw(p) ... ); }
	auto Polo (auto && f, auto && ... p)
	{ return P(*this, fw(f),        fw(p) ... ); }
	
	// operator(): Pass an invocable entity and appropriate arguments to invoke
	// the entity in a context protected by this SyncObject.
	auto operator() (auto && f, auto && ... p)
	{ return C(*this, fw(f),        fw(p) ... ); }
};

template<IsBits DataType> class LockContext<DataType> : public LC<void> {
protected:
	DataType Data;
public:
	LockContext(DataType&& dt = {}) : Data{ move(dt) } {}

	// Marco Polo: Check notes near end of file
	auto Marco(auto && f, auto && ... p)
	{ return M(*this, fw(f), *this, fw(p) ... ); }
	auto Polo (auto && f, auto && ... p)
	{ return P(*this, fw(f),        fw(p) ... ); }

	// operator(): Same as base class but passes Data as first lvalue parameter
	// to invocation
	auto operator() (auto && f, auto && ... p)
	{ return C(*this, fw(f), Data, fw(p) ... ); }

	constexpr operator DataType       & ()       { return Data; }
	constexpr operator DataType const & () const { return Data; }
};

// --------------------------------------------------------------------------- //
// 
// Marco Polo synchronization involves two sides:
// - Marco: Delegating function invoked from the parent thread. The delegated
//   function must start the child thread.
// - Polo: Delegating function invoked from the child thread.
// 
// Marco delegates and then waits for Polo in a synchronized context, whereas
// Polo delegates in the same synchronized context and then notifies Marco
// after leaving that context. This forces Polo to block until Marco is actually
// ready and waiting for its signal, and forces Marco to halt until after Polo's
// work is finished. Any code before the child thread starts and any code in the
// child thread before Polo is synchronized; all other code in Marco or Polo is
// unsynchronized.
// 
// The use case which inspired Marco Polo was to ensure that initialization of
// child thread A using a resource owned by parent thread P HAPPENS-BEFORE the
// use of said resource child thread B:
// 
// 1a. (P) calls Marco with an Action that starts (A).
// 1b. (P) in Marco, waits for the notification.
// --------
// 2a. (A) begins, eventually reaching its Polo call.
// 2b. (A) in Polo, waits for (P) to release the lock.
// 2c. (A) in Polo, executes its Action, in which it uses the protected resource.
// 2d. (A) in Polo, releases the lock, and then notifies (P).
// 2e. (A) at end of Polo, is no longer synchronized, and continues execution.
// --------
// 3a. (P) in Marco, receives the notification from 2d. and continues execution.
// 3b. (P) in Marco, releases the lock that was implicitly reacquired in wait().
// 3c. (P) is no longer synchronized, and continues execution.
// 3d. (P) eventually executes code that starts (B).
// 
// --------------------------------------------------------------------------- //
// 
// Marco Polo can be disrupted by another thread claiming the mutex and doing its
// own Marco or Polo between Polo::unlock and Polo::notify_one. This problem is
// beyond the scope of Marco Polo, as it is intended to synchronize parent and
// child threads between the time the child thread begins and when it calls Polo.
// It is not recommended to use the same LockContext for Marco Polo
// synchronization in parallel threads, but if necessary, it will require some
// higher-order form of synchronization to manage which parent thread is notified
// by which child thread. Could just use an extra mutex, locked at the start of
// Marco and unlocked at the end of Polo. I'll think about it. (TODO)
// 
// --------------------------------------------------------------------------- //
// 
// Also note that this doesn't provide a way to return a value from inside the
// child thread; getting a result requires waiting for the child thread to
// finish, and with the particular way that Marco and Polo are tangled up in each
// other, manually waiting for the thread (whether via wait, join, or anything
// promise- or future-related) will cause deadlock for anything that synchronizes
// with the LockContext in question. The best solution at the moment is to pass
// in a pointer or reference and mutate it wherever desired - by the very design
// of Marco Polo, it'll be ready for any other code that wants to use the
// resulting value so long as Marco or Polo appears before it in code.
// 
// --------------------------------------------------------------------------- //
// 
// Ideas to try:
// - Pass a function to Marco that creates a std::thread and move-returns it, and
//   another that returns it plainly (copy-elision). Join both of them in the
//   parent thread.
//   - Try same with jthread - does it join at the end of Marco or the function
//     that called it?
// 
// --------------------------------------------------------------------------- //
// 
// TODO: Simplify these notes?
// 
// --------------------------------------------------------------------------- //
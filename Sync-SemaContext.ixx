export module Sync : SemaContext;
import : LockContext;
import std;
using namespace std;

/*\
 * SemaContext is a wrapper class template for LockContext, augmenting it with
 * semaphore behavior.
\*/

// --------------------------------------------------------------------------- //
// --- Exports and forward declarations -------------------------------------- //
// --------------------------------------------------------------------------- //

export template<typename> class SemaContext;
export template<typename T> using SC = SemaContext<T>;

template<typename T>
SemaContext(T&&) -> SC<T>;
SemaContext(   ) -> SC<void>;

// --------------------------------------------------------------------------- //
// --- SemaContext class template -------------------------------------------- //
// --------------------------------------------------------------------------- //

template<typename T> class SemaContext : public LockContext<T> {
private:
	LockContext<unsigned> Gate;
public:
	using LockContext<T>::LockContext;
	auto lock   () -> void;
	auto unlock () -> void;
};

// >>>                                                                     <<< //
// --- Member function definitions ------------------------------------------- //
// >>>                                                                     <<< //

template<typename T> auto SemaContext<T>::lock  () -> void {
	Gate.lock();
	if (!Gate) MTX::lock();
	++Gate;
	Gate.unlock();
}

template<typename T> auto SemaContext<T>::unlock() -> void {
	Gate.lock();
	--Gate;
	if (!Gate) MTX::unlock();
	Gate.unlock();
}
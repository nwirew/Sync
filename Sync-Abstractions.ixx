module;

#include "SyncMacros.h"

export module Sync : Abstractions;
import std;
using namespace std;



// BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
// - AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
// --- NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// ----- NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// ------- EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
// --------- RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
// ---------
// >>>>>>>>> Template metaprogramming types <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// ---------
// --------- RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
// ------- EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
// ----- NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// --- NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// - AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
// BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB



Type1(T) using DRef = remove_pointer_t<T>;
Type1(T) using Bare = remove_cvref_t<T>;
Type1(T) using Name = Bare<DRef<T>>;

// Function return type
export Type2(F, ...Ps) using Ret = invoke_result_t<F, Ps...>;



// BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
// - AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
// --- NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// ----- NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// ------- EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
// --------- RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
// ---------
// >>>>>>>>> Concepts <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// ---------
// --------- RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
// ------- EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
// ----- NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// --- NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// - AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
// BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB



// --------------------------------------------------------------------------- //
// --- Module-local concepts ------------------------------------------------- //
// --------------------------------------------------------------------------- //

Type1(T) concept IsPtr = is_pointer_v<T>;
Type1(...Ts) concept AllPtr = (IsPtr<Ts> && ...);

// --------------------------------------------------------------------------- //
// --- Type matching --------------------------------------------------------- //
// --------------------------------------------------------------------------- //

// >>>                                                                     <<< //
// --- Single ---------------------------------------------------------------- //
// >>>                                                                     <<< //

export Type2(A, B) concept Is = same_as<Bare<A>, Bare<B>>;
export Type2(A, B) concept Isnt = !Is<A, B>;
export Type1(T) concept IsVoid = Is<void, T>;
export Type1(T) concept IsBits = !IsVoid<T>;
export Type1(T) concept IsLval = is_lvalue_reference_v<T>;
export Type1(T) concept IsRval = is_rvalue_reference_v<T>;

// >>>                                                                     <<< //
// --- Multiple -------------------------------------------------------------- //
// >>>                                                                     <<< //

Type2(A, B) concept PolymorphOf =
	Is<Name<A>, Name<B>> || is_base_of_v<Name<B>, Name<A>> ||
	Is<A, nullptr_t> && IsPtr<B>;
export Type2(A, ...B) concept Inherits =  (PolymorphOf<A, B> && ...);
export Type2(A, ...B) concept Abherits = !(PolymorphOf<A, B> || ...);

// "PolymorphOf<A, B>": A and B are the same class, or B is a descendant class
// of A. nullptr_t is considered to be a PolymorphOf B if B was explicitly
// stated to be any pointer type.
// "Inherits<A, ...B>": A is a PolymorphOf all ...B.
// "Abherits<A, ...B>": A is not a PolymorphOf any ...B.

// --------------------------------------------------------------------------- //
// --- Parameter pack tests -------------------------------------------------- //
// --------------------------------------------------------------------------- //

export template<typename... Ts, size_t lo = 0, size_t hi = lo>
concept PackSize = lo <= sizeof...(Ts) && sizeof...(Ts) <= hi;

export Type2(...Ts, U) concept Match = sizeof...(Ts) && (Is<Ts, U> && ...);
export Type2(T, ...Us) concept OneIn = sizeof...(Us) && (Is<T, Us> || ...);
export Type2(T, ...Us) concept NotIn = sizeof...(Us) && !OneIn<T, Us...>;

export TypeX(...Ts, bool, ...Us) concept EachIn = (OneIn<Ts, Us...> && ...);
export TypeX(...Ts, bool, ...Us) concept NoneIn = (NotIn<Ts, Us...> && ...);
// EachIn = Ts is a complete subset of Us
// NoneIn = Ts and Us are disjoint sets

// --------------------------------------------------------------------------- //
// --- Function tests -------------------------------------------------------- //
// --------------------------------------------------------------------------- //

export Type2(F, ...Ps) concept Inv = invocable<F, Ps...>;

export Type3(F, R, ...Ps) concept Sig = Is<Ret<F, Ps...>, R>;
export Type3(F, T, ...As) concept IsMemFun =
	requires (F mf, T t, As... as) { (t.*mf)(as...); };
#pragma once

// --- General template shorthands ------------------------------------------- //
#define Type1(X1) \
	template<typename X1>
#define Type2(X1, X2) \
	template<typename X1, typename X2>
#define Type3(X1, X2, X3) \
	template<typename X1, typename X2, typename X3>
#define Type4(X1, X2, X3, X4) \
	template<typename X1, typename X2, typename X3, typename X4>
#define TypeX(T1, S,  T2) template<typename T1, S, typename T2>

// --- Function delegation helpers ------------------------------------------- //
#define DelParams(        F, Ps) F&& fun,         Ps&&... ps
#define DelRet(           F, Ps) Ret<F, Ps...>
#define DelFwd(           F, Ps) forward<F>(fun), forward<Ps>(ps) ...
#define DelInvocation(    F, Ps) invoke(DelFwd(F, Ps))

#define DelTemp(          F, Ps) \
	template<typename... Ps, Inv<Ps...> F>

#define DelRetCon(        F, Ps, RC) \
	requires RC<DelRet(F, Ps)>

#define DelTempWithRetCon(F, Ps, RC) \
	DelTemp(F, Ps) DelRetCon(F, Ps, RC)

// --- With extra arguments -------------------------------------------------- //
#define DelArgsParams(    F, Ps, ...) F&& fun,         __VA_ARGS__, Ps&&... ps
#define DelArgsRet(       F, Ps, ...) Ret<F,           __VA_ARGS__, Ps...>    
#define DelArgsFwd(       F, Ps, ...) forward<F>(fun), __VA_ARGS__, forward<Ps>(ps) ...
#define DelArgsInvocation(F, Ps, ...) invoke(DelArgsFwd(F, Ps, __VA_ARGS__))

#define DelArgsTemp(      F, Ps, ...) \
	template<typename... Ps, Inv<__VA_ARGS__, Ps...> F>

#define DelArgsRetCon(    F, Ps, RC, ...) \
	requires RC<DelArgsRet(F, Ps, __VA_ARGS__)>

#define DelArgsTempWithRetCon(F, Ps, RC, ...) \
	DelArgsTemp(F, Ps, __VA_ARGS__) DelArgsRetCon(F, Ps, RC, __VA_ARGS__)

// --------------------------------------------------------------------------- //
// --- Inheritance helpers --------------------------------------------------- //
// --------------------------------------------------------------------------- //


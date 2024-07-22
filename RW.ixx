export module RW;
import Sync;
import std;
using namespace std;

// TODO:
// - Create specialization of RW<void> which acts as a utility class (e.g.
//   contains mode enum) and template RW<not void> which implements the actual
//   protective behavior. Declare main template with default type = void.
// - Check that RW works as expected with recent changes to LockContext and
//   SemaContext, e.g. check if LockContext references to SemaContext instances
//   are treated as declared; SemaContext should be allowed to be treated as
//   either one.

export enum class RWPref { R, W, F };

export template<IsBits O = char>
class RW final {
private:
	RWPref mode = RWPref::F;

	SemaContext<O> Core;
	LockContext<O> & CoreLck{ Core };
	
	SemaContext<void> CoreGate;
	LockContext<void> & CoreGateLck{ CoreGate };

	LockContext<unsigned> RCensus, WCensus;

	// --- Fair preference ------------------------------------------------ //
	
	SyncTree<SemaContext<O>> DR_PF{
		Core,
		new SyncTree{ CoreGateLck }, nullptr
	};
	
	SyncTree<LockContext<O>> DW_PF{
		CoreLck,
		new SyncTree{ CoreGateLck }, nullptr
	};
	
	// --- Reader's preference -------------------------------------------- //
	
	SyncTree<SemaContext<O>> DR_PR{ Core };
	SyncTree<LockContext<O>> DW_PR{ CoreLck };
	
	// --- Writer's preference -------------------------------------------- //
	
	SyncTree<SemaContext<O>> DR_PW{
		Core,
		new SyncTree{ CoreGateLck }, nullptr
	};
	
	SyncTree<SemaContext<void>> DW_PW{
		CoreGate,
		new SyncTree{ CoreLck }
	};

public:
	LockContext<void> innerModeGate, outerModeGate;
	// GetMode goes through an extra gate because SetMode should be guaranteed
	// essentially immediate access to the protected variable so it can be
	// updated ASAP.
	auto GetMode() -> RWPref {
		static SyncTree context{ outerModeGate,
			new SyncTree { innerModeGate,
				new SyncTree { CoreLck }
			}
		};
		
		static auto retrieve{ [&]() -> RWPref { return mode; } };
		
		return context(retrieve);
	}


	auto SetMode(RWPref const && m) -> void {
		static SyncTree context{ innerModeGate,
			new SyncTree{ CoreLck }
		};
		
		static auto assign{ [&] { mode = m; } };

		cout << ":::: RW type is " << typeid(O).name() << endl;
		cout << ":::: test " << typeid(SyncTree{CoreLck}).name() << endl;

		context(assign);
	}

	RW(O&& o = {}) : Core{ move(o) } {}



public:
	template<typename... P, Inv<O const &, P...> F>
	auto Read(F &&, P &&...) -> Ret<F, O const &, P...>;
	auto Write(O &&) -> void;
};

#pragma region RW public

template<IsBits O>
template<typename... P, Inv<O const &, P...> F>
auto RW<O>::Read(F && f, P && ... p) -> Ret<F, O const &, P...> {
	cout << "RW Read: Getting Mode\n";
	switch (GetMode()) {
	case RWPref::R: cout << "RW Read: Reader's preference\n";
		return DR_PR(forward<F>(f), (O const &)CoreLck, forward<P>(p) ... );
	case RWPref::W: cout << "RW Read: Writer's preference\n"; 
		return DR_PW(forward<F>(f), (O const &)CoreLck, forward<P>(p) ... );
	case RWPref::F: cout << "RW Read: Fair preference\n";
		return DR_PF(forward<F>(f), (O const &)CoreLck, forward<P>(p) ... );
	}
}

template<IsBits O>
auto RW<O>::Write(O && o) -> void {
	auto Op = [&](O const & oIn) { (O &)Core = forward<O const>(oIn); };

	cout << "RW Write: Getting Mode\n";

	switch (GetMode()) {
	case RWPref::R:
		return DW_PR(Op, forward<O>(o));
	case RWPref::W:
		return DW_PW(Op, forward<O>(o));
	case RWPref::F:
		return DW_PF(Op, forward<O>(o));
	}
}

#pragma endregion
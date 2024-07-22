import Sync;
import RW;
import std;
using namespace std;

using Random = random_device;

struct Junk { long double j[0xFFFF]; };

#pragma region Function declarations

auto Print(auto const &) -> void;
auto SectionHeader(string const &) -> void;

// --------------------------------------------------------------------------- //

auto FairTest(int const &, char) -> void;
auto ReaderTest(int const &) -> void;
auto WriterTest(int const &, char) -> void;

// --------------------------------------------------------------------------- //

template<typename T>
auto LockContextExam(LockContext<T>&, jthread&, unsigned&) -> void;

template<typename T> auto marco_work_1(LockContext<T>&, jthread&) -> void;
template<typename T> auto jthread_work_1(LockContext<T>&) -> void;
auto polo_work_1() -> void;

template<typename T>
auto marco_work_2(LockContext<T>&, jthread&, unsigned&) -> unsigned;
template<typename T>
auto jthread_work_2(LockContext<T>&, unsigned&) -> unsigned;
auto polo_work_2() -> unsigned;

auto lc_invoke_1_0(auto&) -> void;
auto lc_invoke_1_0_v() -> void;
auto lc_invoke_1_3(auto&, char, string, unsigned) -> void;
auto lc_invoke_1_3_v(char, string, unsigned) -> void;

// --------------------------------------------------------------------------- //

template<IsContext C, typename T> auto SyncTreeExam(SyncTree<C>&) -> void;
template<IsContext C, typename T> auto InvokeTest(SyncTree<C>&) -> void;

#pragma endregion



// --- Test start ------------------------------------------------------------ //

auto main(int argc, char** argv) -> int {

	LockContext s{ "LockContext string text"s };
	LockContext x{ 33 };
	LockContext v1, v2, v3, v4, v5, v6, v7, v8, v9;
	
	v1([]{});
	
	#pragma region LockContext testing

	SectionHeader("SYNCCONTEXT TESTS");

	jthread jt;
	unsigned u;

	LockContextExam(s, jt, u);
	LockContextExam(v1, jt, u);
	s(lc_invoke_1_0<string>);
	v1(lc_invoke_1_0_v);
	s(lc_invoke_1_3<string>, 'x', "asdfasdfasdf", 123);
	v1(lc_invoke_1_3_v, 'z', "jkl;jkl;jkl;jkl;", 789);

	#pragma endregion

	// ----------------------------------------------------------------------- //

	SemaContext sc1{ "SemaContext string text"s };
	SemaContext sc2{ 4u };
	SemaContext sc3;

	#pragma region SemaContext testing

	SectionHeader("SEMACONTEXT TYPE TEST");
	
	sc1([](string&){ cout << "sc1 invoked\n"; });
	sc3([](){ cout << "sc3 invoked\n"; });
	sc1([](string&) -> unsigned { cout << "sc1 returning 7u\n"; return 7u; });
	sc3([]() -> unsigned { cout << "sc3 returning 7u\n"; return 7u; });
	sc2(lc_invoke_1_0<unsigned>);
	sc2(lc_invoke_1_3<unsigned>, 'c', "yuioiuyuio", 555);

	#pragma endregion

	// ----------------------------------------------------------------------- //

	SyncTree k{ v1 };

	SyncTree w{ v1,
		new SyncTree{ sc1, // 2 args
		new SyncTree{ v3,
		new SyncTree{ x,
		new SyncTree{ v4, // 4 args
			new SyncTree{ v5 }, // 1 arg
			new SyncTree{ v6, // 3 args
				new SyncTree{ v7 },
				new SyncTree{ v8 }
			},
			new SyncTree{ v9 }
	}}}}};

	SyncTree y{ x, new SyncTree{ v8, new SyncTree{ v1 } } };

	#pragma region SyncTree testing

	SectionHeader("SYNCTREE TESTS");

	// Root is LockContext<void>, string is nested inside
	SyncTreeExam<LockContext<void>, string>(w);
	// Root is int, use it
	SyncTreeExam<LockContext<int>, int>(y);

	#pragma endregion

	// ----------------------------------------------------------------------- //

	char char_q{ 'q' };
	string g{ "goo"s };
	RW<int> RW_int;

	#pragma region RW testing

	SectionHeader("RW TESTS");
	
	// TODO: Check these
	RW_int.SetMode(RWPref::F);
	cout << endl;
	RW_int.Write(1);
	cout << endl;
	RW_int.Read(FairTest, 'n');
	
	RW_int.SetMode(RWPref::R);
	RW_int.Write(2);
	RW_int.Read(ReaderTest);
	
	RW_int.SetMode(RWPref::W);
	RW_int.Write(3);
	RW_int.Read(WriterTest, 's');
	
	#pragma endregion

	// ----------------------------------------------------------------------- //

	SectionHeader("FINISHED");
	
	return 0;
}

/*\
 * TODO:
 * - Learn how to write better tests
 * - Test Sync
 * - Test RW
\*/

template<typename... T>
concept Printable = requires(T... t){ ((cout << t), ...); };

// Can't declare inside Print because each auto-template would get a unique
// instance
mutex printmutex{};
auto Print(auto const & thing) -> void requires Printable<decltype(thing)> {
	printmutex.lock();
	cout << thing << endl;
	printmutex.unlock();
}

string hr{ "----------------------------------------------------------------" };
auto SectionHeader(string const & title) -> void {
	Print(hr + '\n' + title);
}

// --------------------------------------------------------------------------- //

#pragma region RW Tests: Fair, readers preference, writers preference

auto FairTest(int const & q, char c) -> void {
	cout << "fair preference test sees " << q << " and '" << c << "'\n";
}

auto ReaderTest(int const & q) -> void {
	cout << "reader preference test sees " << q << endl;
}

auto WriterTest(int const & q, char c) -> void {
	cout << "writer preference test sees " << q << " and '" << c << "'\n";
}

#pragma endregion

// --------------------------------------------------------------------------- //

template<typename T>
auto LockContextExam(LockContext<T>& lc, jthread& jt, unsigned& u) -> void {
	SectionHeader("Using "s + typeid(LockContext<T>).name());
	using RType1 = Ret<decltype(marco_work_1<T>), LockContext<T>&, jthread&>;
	using RType2 = Ret<decltype(marco_work_2<T>), LockContext<T>&, jthread&, unsigned&>;

	SectionHeader("+++ Marco Polo");

	SectionHeader("- returning <"s + typeid(RType1).name() + "> (are numbers ordered?)"s);
	lc.Marco(marco_work_1<T>, ref(jt));

	SectionHeader("- returning <"s + typeid(RType2).name() + "> (are they all the same?)"s);
	lc.Marco(marco_work_2<T>, ref(jt), ref(u));
	Print("Result in parent thread: " + to_string(u));
}

#pragma region LockContext<T> test: return nothing

template<typename T>
auto marco_work_1(LockContext<T>& cref, jthread& jtref) -> void {
	cout << "P1: Fully synchronized" << endl;
	jtref = jthread{ jthread_work_1<T>, ref(cref) };
	Print("P2: Synchronized with parent, but not child");
}

template<typename T>
auto jthread_work_1(LockContext<T>& cref) -> void {
	Print("C3: Not synchronized with parent, but inside critical section");
	cref.Polo(polo_work_1);
	Print("C?: Unsynchronized");
}

auto polo_work_1() -> void {
	cout << "C4: Working; unlock and notify after this" << endl;
}

auto lc_invoke_1_0(auto&) -> void {
	cout << "LockContext<T> invoked with no parameters" << endl;
}

auto lc_invoke_1_0_v() -> void {
	cout << "LockContext<void> invoked with no parameters" << endl;
}

auto lc_invoke_1_3(auto&, char c, string s, unsigned u) -> void {
	cout << "LockContext<T> invoked with 3 parameters" << endl;
	cout << "- Saw " << c << ", " << s << ", and " << u << endl;
}

auto lc_invoke_1_3_v(char c, string s, unsigned u) -> void {
	cout << "LockContext<void> invoked with 3 parameters" << endl;
	cout << "- Saw " << c << ", " << s << ", and " << u << endl;
}

#pragma endregion

#pragma region LockContext<T> test: return non-void type

template<typename T>
auto marco_work_2(LockContext<T>& lb, jthread& jt, unsigned& rv) -> unsigned {
	jt = jthread{ jthread_work_2<T>, ref(lb), ref(rv) };
	return rv;
}

template<typename T>
auto jthread_work_2(LockContext<T>& lb, unsigned& rv) -> unsigned {
	rv = lb.Polo(polo_work_2);
	Print("jthread_work_2 got " + to_string(rv));
	return rv;
}

auto polo_work_2() -> unsigned {
	static uniform_int_distribution<unsigned> d{ 0,999 };
	static Random rd{};
	unsigned ret_value = d(rd);
	Print("Polo: Returning " + to_string(ret_value));
	return ret_value;
}

#pragma endregion

// --------------------------------------------------------------------------- //

template<IsContext C, typename T>
auto SyncTreeExam(SyncTree<C>& st) -> void {
	SectionHeader("+++ "s + typeid(st).name() + " Tests"s);
	InvokeTest<C, T>(st, 0);
}

#pragma region SyncTree<T> test: Send and operator()

static auto const sees = [](auto& t) { cout << "lambda sees '" << t << "'\n"; };
static auto const guessso = [] { cout << "I guess so" << endl; };
static auto const packprint = [](auto&&... ps) requires Printable<decltype(ps)...> {
	cout << "Maybe: ";
	((cout << '(' << ps << ") "), ...);
	cout << endl;
};

template<IsContext C, typename T>
auto InvokeTest(SyncTree<C>& st, unsigned n) -> void {
	SectionHeader("- Send "s + typeid(T).name() + " 0 To lambda (does it match the declared value?)"s);
	st.Send<T>(n).To(sees);

	SectionHeader("- operator(), no arguments (does it compile and run?)"s);
	st(guessso);

	SectionHeader("- operator(), parameter pack (does it match the source code?)"s);
	st(packprint, 55u, "hello i am string", 'x');
}

#pragma endregion
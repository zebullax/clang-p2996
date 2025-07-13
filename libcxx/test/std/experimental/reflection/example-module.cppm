module;
#include <meta>
#include <tuple>

export module Example;
namespace Example {
                              // ================
                              // Null reflections
                              // ================

export constexpr auto rNull = decltype(^^::){};

                         // ===========================
                         // Reflections of type aliases
                         // ===========================

export using Alias = int;
export constexpr auto rAlias = ^^Alias;

                           // ======================
                           // Reflections of objects
                           // ======================

static int obj = 13;
export constexpr auto rObj = std::meta::reflect_object(obj);

                            // =====================
                            // Reflections of values
                            // =====================

export constexpr auto rValue = std::meta::reflect_constant(1);
export constexpr auto rRefl = std::meta::reflect_constant(rValue);
export constexpr auto Splice = [:rRefl:];

                          // ========================
                          // Reflections of variables
                          // ========================

export int v42 = 42;
export constexpr auto r42 = ^^v42;

                          // ========================
                          // Reflections of templates
                          // ========================

export template <auto V> int TVar = -V;
export constexpr auto rTVar = ^^TVar;

export template <typename T, auto M> auto fn(const T &t) {
  return t.[:M:];
}

                          // =========================
                          // Reflections of namespaces
                          // =========================

export constexpr auto rGlobalNS = ^^::;

                       // ==============================
                       // Reflections of base specifiers
                       // ==============================
export struct Empty {};
export struct Base {
  static constexpr int K = 12;
};
export struct Child : private Empty, Base {};
constexpr auto ctx = std::meta::access_context::unchecked();
export constexpr auto rBase1 = bases_of(^^Child, ctx)[0];
export constexpr auto rBase2 = bases_of(^^Child, ctx)[1];

                      // =================================
                      // Reflections of data members specs
                      // =================================

export constexpr auto rTDMS = data_member_spec(^^int, {.name="test"});

                            // ====================
                            // Expansion statements
                            // ====================

void test() {
  // Iterating expansion statement.
  constexpr static auto sequence = std::array{0, 1, 2, 3, 4};
  template for (constexpr auto i : sequence) { (void) i; }

  // Destructurable expansion statement.
  constexpr auto tup = std::make_tuple(1, true, 'c');
  template for (constexpr auto i : tup) { (void) i; }

  // Enumerating expansion statement.
  template for (auto i : {1, 2, 3}) { (void) i; }
}

}  // namespace Example

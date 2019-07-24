#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <typeinfo>
#include <type_traits>
#include <variant>
#include "prettyprint.hpp"
#include "doctest.h"

// Helper for variant visting
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// Recursive visit to hide access to data
template <typename Visitor, typename Variant>
decltype(auto) visit_recursively(Visitor&& visitor, Variant variant) {
  return std::visit (std::forward<Visitor>(visitor),
                     std::forward<Variant>(variant).data);
}

// A variant based implementation of a sequence that allows
// recursive data structures.
namespace impl {
  struct wrapper;
  using vec = std::vector<wrapper>;
  using Sequence = std::variant<int, vec>;

  struct wrapper {
    Sequence data;

    template <typename... Ts>
    wrapper(Ts&&... xs)
      : data{std::forward<Ts>(xs)...} {}
  };

  Sequence operator+ (Sequence lhs, Sequence rhs) {
    return std::visit( [](auto const &l, auto const &r) -> Sequence {
        return l + r;
    }, lhs, rhs);
  }

  bool operator== (Sequence lhs, Sequence rhs) {
    auto is_equal = [&](Sequence lhs, Sequence rhs) -> bool {
      // Both vecs
      if (std::holds_alternative<vec>(lhs) && std::holds_alternative<vec>(rhs)) {
        // Not same size can't be equivalent
        if (std::get<vec>(lhs).size() != std::get<vec>(rhs).size())
          return false;
        else {
          std::vector<bool> eq {};
          std::transform(std::get<vec>(lhs).begin(),
                         std::get<vec>(lhs).end(),
                         std::get<vec>(rhs).begin(),
                         std::back_inserter(eq),
                         [](const auto& l, const auto& r) -> bool {
                           return l.data == r.data; }
              );
          return std::all_of(eq.begin(), eq.end(), [](bool b){ return b;});
        }
      } else if (std::holds_alternative<int>(lhs) && std::holds_alternative<int>(rhs)) {
        return std::get<int>(lhs) == std::get<int>(rhs);
      } else {
        return false;
      }
    };
    return is_equal(lhs, rhs);
  }

  bool operator!= (Sequence lhs, Sequence rhs) {
    return !(lhs == rhs);
  }

  std::ostream &operator<< (std::ostream &os, Sequence s) {
    std::string seq_string{};
    struct build_string {
      std::string& str;
      build_string (std::string& _str) : str(_str) {}
      void operator() (int x) { str += std::to_string(x); }
      void operator() (const vec& v) {
        str += "[";
        for (int i{0}; i < v.size(); ++i) {
          visit_recursively(*this, v[i]);
          if (i != v.size()-1) str += ", ";
        }
        str += "]";
      }
    };
    std::visit(build_string(seq_string), s);
    return os << seq_string;
  }
}
using impl::Sequence;
using impl::vec;

// Print a sequence, complete with brackets
struct seq_print {
  void operator() (int x) { std::cout << x; }
  void operator() (const vec& v) {
    std::cout << '[';
    for (int i{0}; i < v.size(); ++i) {
      visit_recursively(*this, v[i]);
      if (i != v.size()-1) std::cout << ", ";
    }
    std::cout << ']';
  }
}seq_print;

void print(Sequence s) {
  std::visit(seq_print, s);
  std::cout << '\n';
}


// NTD: Normalise Transpose Distribute

// Normalise the length of two containers by repeating elements
// of the smaller container.
template <typename T, typename V>
constexpr void normalise(T &a, V &b) {
  auto gen = [&](const auto& v, int& i) {
    if (i == v.size()) i = 0;
    return v.at(i++);
  };

  auto norm = [&](auto& v, int n) {
    int index{0};
    auto g = std::bind(gen, v, index);
    v.resize(v.size() + n);
    std::generate(v.end()-n, v.end(), g);
  };

  int diff = a.size() - b.size();

  if (diff < 0)
    norm(a, abs(diff));
  else if (diff > 0)
    norm(b, diff);
}


Sequence repeat(Sequence &s, int n) {
  if (n < 2) return s;
  return Sequence {vec (n, s)};
}

// not used
/*
template <typename T, typename... Args>
int minimum(T &a, Args... args) {
  return std::min(int(size(a)), minimum(args...));
}
*/

int order(Sequence &v) {
  struct order_struct{
    int operator() (int x) { return 0; }
    int operator() (vec& v) {
      std::vector<int> depth;
      for (auto& x : v)
        depth.push_back(order(x.data));
      return 1 + *max_element(depth.begin(), depth.end());
    }
  };
  int n = std::visit(order_struct{}, v);

  return n;
}

/*
void norme(Sequence& a) {
  int a_size;
  if (std::holds_alternative<vec>(a))
    a_size = std::get<vec>(a).size();
  else a_size = 1;
  if (order(a) <= 1) return;
  std::vector<int> orders (a_size, 0);
  int max_order = std::max(std::transform(
        std::get<vec>(a).begin(), 
        std::get<vec>(a).end(),
        orders.begin(),
        [](const auto& a) { return order(a); } ));
}
*/

void norme(Sequence& a, Sequence& b) {
  /*
  std::cout << "[norm] a: "; print(a);
  std::cout << "       b: "; print(b);
  std::cout << "       level: " << level << '\n';
  */

  int a_size, b_size;
  if (std::holds_alternative<vec>(a))
    a_size = std::get<vec>(a).size();
  else a_size = 1;
  if (std::holds_alternative<vec>(b))
     b_size = std::get<vec>(b).size();
  else b_size = 1;
  if ((order(a) <= 1) && (order(a) == order(b)) && (a_size == b_size)) {
    return;
  } else if ((order(a) > 1) && (order(a) == order(b)) && (a_size == b_size)) {
    // done at this level
    for (int i{0}; i < a_size; ++i)
      norme(std::get<vec>(a).at(i).data, std::get<vec>(b).at(i).data);
    return;
  } else if ((order(a) == order(b)) && (a_size != b_size)) {
    normalise(std::get<vec>(a),std::get<vec>(b));
  } else if ((a_size == b_size) && (order(a) != order(b))) {
    if (order(a) < order(b))
      a = repeat(a, b_size);
    else
      b = repeat(b, a_size);
  } else {
    if (order(a) < order(b))
      a = repeat(a, b_size);
    else
      b = repeat(b, a_size);
  }
  norme(a, b);
}

TEST_CASE("testing normalisation") {
  Sequence a,b;

  SUBCASE("order 1, same length") {
    a = vec{2,3,4};
    b = vec{3,2,6};
    norme(a,b);

    CHECK(a == Sequence { vec{2,3,4} });
    CHECK(b == Sequence { vec{3,2,6} });
  }

  
  SUBCASE("order 1") {
    a = vec{1,2,3,4,5};
    b = vec{6,7};
    norme(a,b);

    CHECK(a == Sequence { vec{1,2,3,4,5} });
    CHECK(b == Sequence { vec{6,7,6,7,6} });
  }

  SUBCASE("order 2") {
    a = vec{vec{2,3},vec{5,7}};
    b = vec{vec{8,9},vec{2,1},vec{7,6}};
    norme(a,b);

    CHECK(a == Sequence { vec{vec{2,3},vec{5,7},vec{2,3}} });
    CHECK(b == Sequence { vec{vec{8,9},vec{2,1},vec{7,6}} });
  }

  SUBCASE("order delta 1, different length") {
    a = vec{6, 7};
    b = vec{8, vec{3, 4}, 1};
    norme(a,b);

    CHECK(a == Sequence { vec{vec{6,7},vec{6,7},vec{6,7}} });
    CHECK(b == Sequence { vec{vec{8,8},vec{3,4},vec{1,1}} });
  }
}


// Distribute
Sequence transpose_distribute(Sequence& a, Sequence& b) {
  int _order = order(a);
  Sequence result = vec{};

  if (_order == 1) {
    std::transform(
        std::get<vec>(a).begin(), 
        std::get<vec>(a).end(),
        std::get<vec>(b).begin(),
        std::back_inserter(std::get<vec>(result)),
        [](const auto& a, const auto& b) -> Sequence {
          return a.data + b.data; } );
  } else {
    int a_size = std::get<vec>(a).size();
    for (int i{0}; i < a_size; ++i) {
      std::get<vec>(result).push_back(transpose_distribute(
          std::get<vec>(a).at(i).data, std::get<vec>(b).at(i).data));
    }
  }
  return result;
}

/*
int main() {
  std::vector<int> vec1 {1,2,3,4,5,6,7,8};
  std::vector<double> vec2 {4,5,3,2};
  std::vector<int> vec3 {1,2};
  std::vector<std::vector<int>> vec4 { {3,4},{5,6} };
  std::vector<std::vector<int>> vec5 { {3,4},{5,6,7} };
  int num = 345;
  std::variant<int,std::string> var1 {4};

  Sequence c = 5;
  Sequence d = vec{6, 7};
  Sequence e = vec{d, c};
  Sequence f = vec{c, c, e, d, c};
  Sequence g = vec{8, 4, 5, 2, 1};
  Sequence h = vec{8, vec{3, 4}, 1};

  //print(c);
  //print(d);
  //print(e);
  //print(f);
  //print(g);
  //print(h);
  
  Sequence result = vec{};

  // Attempt to normalise two sequences
  Sequence a = d;
  Sequence b = h;
  std::cout << "a: "; print(a);
  std::cout << "b: "; print(b);
  norme(a,b);
  std::cout << "normalised a: "; print(a);
  std::cout << "normalised b: "; print(b);
  result = transpose_distribute(a,b);
  std::cout << "result:       "; print(result);

  a = vec{1,2,3};
  b = vec{5, vec{3,4}, vec{7,6}};
  std::cout << "a: "; print(a);
  std::cout << "b: "; print(b);
  norme(a,b);
  std::cout << "normalised a: "; print(a);
  std::cout << "normalised b: "; print(b);
  result = transpose_distribute(a,b);
  std::cout << "result:       "; print(result);

  a = vec{1,2};
  b = vec{5, vec{3,4}, vec{7,6}};
  std::cout << "a: "; print(a);
  std::cout << "b: "; print(b);
  norme(a,b);
  std::cout << "normalised a: "; print(a);
  std::cout << "normalised b: "; print(b);
  result = transpose_distribute(a,b);
  std::cout << "result:       "; print(result);
  
  a = vec{1,2,3};
  b = vec{4,5,6};
  std::cout << "a: "; print(a);
  std::cout << "b: "; print(b);
  norme(a,b);
  std::cout << "normalised a: "; print(a);
  std::cout << "normalised b: "; print(b);
  result = transpose_distribute(a,b);
  std::cout << "result:       "; print(result);

  
  a = vec{1,vec{4,5},3};
  std::cout << "a: "; print(a);
  norme(a,a);
  std::cout << "normalised a: "; print(a);
  result = transpose_distribute(a,a);
  std::cout << "result:       "; print(result);
 
}
*/


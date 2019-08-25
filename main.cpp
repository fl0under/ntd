#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
//#define DOCTEST_CONFIG_IMPLEMENT

#include <vector>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <typeinfo>
#include <type_traits>
#include <variant>
#include "doctest.h"
#include "prettyprint.hpp"

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

  Sequence operator* (Sequence lhs, Sequence rhs) {
    return std::visit( [](auto const &l, auto const &r) -> Sequence {
        return l * r;
    }, lhs, rhs);
  }

  bool operator== (Sequence lhs, Sequence rhs) {
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
  }

  bool operator!= (Sequence lhs, Sequence rhs) {
    return !(lhs == rhs);
  }

  // Enable printing of a Sequence
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


// NTD: Normalise Transpose Distribute

// Normalise the length of two containers by repeating elements
// of the smaller container.
template <typename T, typename V>
constexpr void repeat_elements(T &a, V &b) {
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

template <typename T>
constexpr void repeat_elements1(T &a, int b) {
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

  int diff = b - a.size();

  if (diff == 0)
    return;
  else if (diff > 0)
    norm(a, diff);
}


Sequence clone_elements(Sequence &s, int n) {
  if (n < 2) return s;
  return Sequence {vec (n, s)};
}

void clone_elements1(Sequence &s, int n) {
  if (n < 2) return;
  s = Sequence {vec (n, s)};
}


int order(const Sequence &v) {
  return std::visit( overloaded {
        [] (const int x) { return 0; },
        [] (const vec& v) {
          std::vector<int> depth;
          for (auto& x : v)
            depth.push_back(order(x.data));
          return 1 + *max_element(depth.begin(), depth.end());
        }
    }, v);
}

// Get the max length at each level/depth
void get_length(std::vector<int>& lengths, int order, Sequence& s) {
  if (std::holds_alternative<int>(s)) return;
  if (order > lengths.size()) lengths.push_back(0);
  int n = std::get<vec>(s).size();
  if (n > lengths.at(order-1))
    lengths.at(order-1) = n;
  for (auto& x : std::get<vec>(s))
    get_length(lengths, order+1, x.data);
}

std::vector<int> get_lengths(Sequence &s) {
  std::vector<int> lengths {0};
  get_length(lengths, 1, s);
  return lengths;
}

TEST_CASE("getting lengths") {
  Sequence a;

  SUBCASE("order 1") {
    a = vec{2,3,4};
    auto lengths = get_lengths(a);
    CHECK(lengths == std::vector<int>{3});
  }

  SUBCASE("order 2") {
    a = vec{2,3,vec{4,5}};
    auto lengths = get_lengths(a);
    CHECK(lengths == std::vector<int>{3,2});
  }

  SUBCASE("order 2") {
    a = vec{vec{1,2,3},3,vec{4,5},vec{5}};
    auto lengths = get_lengths(a);
    CHECK(lengths == std::vector<int>{4,3});
  }

  SUBCASE("order 3") {
    a = vec{2,3,vec{2,3,vec{7,8}},vec{4,5}};
    auto lengths = get_lengths(a);
    CHECK(lengths == std::vector<int>{4,3,2});
  }

  SUBCASE("order 0") {
    a = 73;
    auto lengths = get_lengths(a);
    CHECK(lengths == std::vector<int>{0});
  }
}

void copy_elements(
    std::vector<int>& norm_s, std::vector<int>& lengths, int order, Sequence s, int& start_pos) {

  //if (std::holds_alternative<int>) return;
  bool done {false};
  int n{0};

  if (order > lengths.size()) {
    // Must have reached a terminal vector
    done = true;
    n = 1;
  } else {
    // Make the current element the right length
    //int n = lengths.at(order-1) - std::get<vec>(s).size();
    n = std::accumulate( lengths.begin() + order-1,  lengths.end(),
        1, std::multiplies<int>() );
    // For a vector, repeat elements until have the required length
    if (std::holds_alternative<vec>(s))
      repeat_elements1(std::get<vec>(s), lengths.at(order-1));
    // For a number, just clone it to get required length
    else {
      clone_elements1(s, n); // not tested this one
      done = true;
    }
  }

  if (done) {
    // Copy it across to the final vector
    if (std::holds_alternative<vec>(s)) {
      //std::transform(std::get<vec>(s).begin(), std::get<vec>(s).end(), 
      //    norm_s.begin() + start_pos, [](impl::wrapper v) -> int { return std::get<int>(v.data); });
      for (int i{0}; i < n; ++i) {
        norm_s.at(start_pos+i) = std::get<int>( std::get<vec>(s).at(i).data );
      }
    } else {
      norm_s.at(start_pos) = std::get<int>(s);
    }
    start_pos += n; 
  } else {
    for (auto& x : std::get<vec>(s))
      copy_elements(norm_s, lengths, order+1, x.data, start_pos);
  }
}

std::vector<int> normalise2(Sequence s, std::vector<int> lengths) {
  std::vector<int> norm_s ( std::accumulate(
        lengths.begin(), lengths.end(), 1, std::multiplies<int>()) );
  int start_pos {0};
  copy_elements(norm_s, lengths, 1, s, start_pos);
  return norm_s;
}

TEST_CASE("normalise2") {
  Sequence a;

  SUBCASE("order 1") {
    a = vec{2,3,4};
    auto lengths = get_lengths(a);
    auto normalised = normalise2(a, lengths);
    CHECK(normalised == std::vector<int>{2,3,4});
  }

  SUBCASE("order 2") {
    a = vec{2,3,vec{7,8},4};
    auto lengths = get_lengths(a);
    auto normalised = normalise2(a, lengths);
    CHECK(normalised == std::vector<int>{2,2,3,3,7,8,4,4});
  }

  SUBCASE("order 3") {
    a = vec{vec{6,9,3},3,vec{7,8},4};
    auto lengths = get_lengths(a);
    auto normalised = normalise2(a, lengths);
    CHECK(normalised == std::vector<int>{6,9,3,3,3,3,7,8,7,4,4,4});
  }
}

void normalise1(Sequence& a) {
  int a_size;
  if (std::holds_alternative<vec>(a))
    a_size = std::get<vec>(a).size();
  else a_size = 1;

  if (order(a) <= 1) return;
  // Get the order of each element
  std::vector<int> elem_orders (a_size);
  std::transform(
      std::get<vec>(a).begin(),
      std::get<vec>(a).end(),
      elem_orders.begin(),
      [](auto &x) { return order(x.data); });
  //for (auto& x : elem_orders) std::cout << x << ' '; std::cout << '\n';
  // Is the order of each element at this level the same?
  bool equiv_order = std::all_of(
      elem_orders.begin(), elem_orders.end(),
      [&](auto &x) { return x == elem_orders.at(0); }
      );
  // If not, make it so
  if (!equiv_order) {
    auto max_order = std::max_element(
        elem_orders.begin(), elem_orders.end());
    //std::cout << "max_order is " << *max_order;
    int max_pos = std::distance(elem_orders.begin(), max_order);
    //std::cout << " at position " << max_pos << '\n';
    for (int i{0}; i < a_size; ++i)
      if (i != max_pos) std::get<vec>(a).at(i).data = clone_elements(std::get<vec>(a).at(i).data,
          std::get<vec>(std::get<vec>(a).at(max_pos).data).size());
  }
  // Need to make all elements same max_size
  // What is the size of the biggest element at this level?
  /*
  auto max_size = std::max_element(std::get<vec>(a).begin(),
      std::get<vec>(a).end(),
      [](auto &l, auto &r) { return std::get<vec>(l.data).size() < std::get<vec>(r.data).size(); }
      );
  // Need to make all elements same max_size
  for (int i{0}; i < a_size; ++i)
    repeat_elements1(std::get<vec>(a), std::get<vec>((*max_size).data).size());
*/
  return;
}

void normalise(Sequence& a, Sequence& b) {
  /*
  std::cout << "[norm] a: " << a << '\n';
  std::cout << "       b: " << b << '\n';;
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
      normalise(std::get<vec>(a).at(i).data, std::get<vec>(b).at(i).data);
    return;
  } else if ((order(a) == order(b)) && (a_size != b_size)) {
    repeat_elements(std::get<vec>(a),std::get<vec>(b));
  } else if ((a_size == b_size) && (order(a) != order(b))) {
    if (order(a) < order(b))
      a = clone_elements(a, b_size);
    else
      b = clone_elements(b, a_size);
  } else {
    if (order(a) < order(b))
      a = clone_elements(a, b_size);
    else
      b = clone_elements(b, a_size);
  }
  normalise(a, b);
}

TEST_CASE("testing normalisation") {
  Sequence a,b;

  SUBCASE("order 1, same length") {
    a = vec{2,3,4};
    b = vec{3,2,6};
    normalise(a,b);

    CHECK(a == Sequence { vec{2,3,4} });
    CHECK(b == Sequence { vec{3,2,6} });
  }

  SUBCASE("order 2, same sequence") {
    a = vec{vec{3,2},1};
    normalise1(a);

    CHECK(a == Sequence { vec{vec{3,2},vec{1,1}} });
  }
  
  SUBCASE("order 1") {
    a = vec{1,2,3,4,5};
    b = vec{6,7};
    normalise(a,b);

    CHECK(a == Sequence { vec{1,2,3,4,5} });
    CHECK(b == Sequence { vec{6,7,6,7,6} });
  }

  SUBCASE("order 2") {
    a = vec{vec{2,3},vec{5,7}};
    b = vec{vec{8,9},vec{2,1},vec{7,6}};
    normalise(a,b);

    CHECK(a == Sequence { vec{vec{2,3},vec{5,7},vec{2,3}} });
    CHECK(b == Sequence { vec{vec{8,9},vec{2,1},vec{7,6}} });
  }

  SUBCASE("order 3") {
    a = vec{vec{vec{2,2},vec{3,3}}, vec{vec{5,5},vec{7,7}}};
    b = vec{vec{vec{8,8},vec{9,9}}, vec{vec{2,2},vec{1,1}}, vec{vec{7,7},vec{6,6}}};
    normalise(a,b);

    CHECK(a == Sequence { vec{vec{vec{2,2},vec{3,3}}, vec{vec{5,5},vec{7,7}}, vec{vec{2,2},vec{3,3}}}});
    CHECK(b == Sequence { vec{vec{vec{8,8},vec{9,9}}, vec{vec{2,2},vec{1,1}}, vec{vec{7,7},vec{6,6}}}});
  }

  SUBCASE("order delta 1, different length") {
    a = vec{6, 7};
    b = vec{8, vec{3, 4}, 1};
    normalise(a,b);

    CHECK(a == Sequence { vec{vec{6,7},vec{6,7},vec{6,7}} });
    CHECK(b == Sequence { vec{vec{8,8},vec{3,4},vec{1,1}} });
  }

  SUBCASE("order delta 1, same length") {
    a = vec{1,2,3};
    b = vec{5, vec{3,4}, vec{7,6}};
    normalise(a,b);

    CHECK(a == Sequence { vec{vec{1,2,3},vec{1,2,3},vec{1,2,3}} });
    CHECK(b == Sequence { vec{vec{5,5,5},vec{3,4,3},vec{7,6,7}} });
  }

  SUBCASE("order delta 2, same length") {
    a = vec{1,2,3};
    b = vec{5, vec{vec{3,0},4}, vec{7,6}};
    normalise(a,b);

    CHECK(a == Sequence { vec{ vec{vec{1,2,3},vec{1,2,3},vec{1,2,3}},
                               vec{vec{1,2,3},vec{1,2,3},vec{1,2,3}},
                               vec{vec{1,2,3},vec{1,2,3},vec{1,2,3}}
                          }
                        });
    CHECK(b == Sequence { vec{ vec{vec{5,5,5},vec{5,5,5},vec{5,5,5}},
                               vec{vec{3,0,3},vec{4,4,4},vec{3,0,3}},
                               vec{vec{7,6,7},vec{7,6,7},vec{7,6,7}}
                          }
                        });
  }

}

namespace sequence_operator {
  Sequence plus(impl::wrapper& a, impl::wrapper& b) {
    return a.data + b.data;
  }

  Sequence multiplies(impl::wrapper& a, impl::wrapper& b) {
    return a.data * b.data;
  }
}

// Pass functions using template params for now,
// maybe change to function_view from here later.
// https://vittorioromeo.info/index/blog/passing_functions_to_functions.htm
template <typename TF>
Sequence transpose_distribute(Sequence& a, Sequence& b, TF&& func) {
  int _order = order(a);
  Sequence result = vec{};

  if (_order == 1) {
    std::transform(
        std::get<vec>(a).begin(), 
        std::get<vec>(a).end(),
        std::get<vec>(b).begin(),
        std::back_inserter(std::get<vec>(result)),
        func
        );
  } else {
    int a_size = std::get<vec>(a).size();
    for (int i{0}; i < a_size; ++i) {
      std::get<vec>(result).push_back(transpose_distribute(
          std::get<vec>(a).at(i).data, std::get<vec>(b).at(i).data, func));
    }
  }
  return result;
}


TEST_CASE("testing transpose-distribute") {
  Sequence a,b,result;

  SUBCASE("order 1, plus") {
    a = vec{2,3,4};
    b = vec{3,2,6};
    result = transpose_distribute(a,b, sequence_operator::plus);

    CHECK(result == Sequence { vec{5,5,10} });
  }

  SUBCASE("order 1, multiply") {
    a = vec{2,3,4};
    b = vec{3,2,6};
    result = transpose_distribute(a,b, sequence_operator::multiplies);

    CHECK(result == Sequence { vec{6,6,24} });
  }

  SUBCASE("order 2, multiply") {
    a = vec{vec{1,2},vec{3,4}};
    b = vec{vec{5,6},vec{7,8}};
    result = transpose_distribute(a, b, sequence_operator::multiplies);

    CHECK(result == Sequence { vec{vec{5,12},vec{21,32}} });
  }

  SUBCASE("order 1, multiply") {
    a = vec{3,3,3};
    b = vec{-1,3,2};
    result = transpose_distribute(a, b, sequence_operator::multiplies);

    CHECK(result == Sequence { vec{-3,9,6} });
  }
}

template <typename TF>
Sequence ntd(Sequence& a, Sequence& b, TF&& func) {
  normalise(a,b);
  return transpose_distribute(a,b,func);
}

TEST_CASE("testing transpose-distribute") {
  Sequence a,b,result;

  SUBCASE("order delta 1, multiply") {
    a = 3;
    b = vec{-1,3,2};
    result = ntd(a, b, sequence_operator::multiplies);

    CHECK(result == Sequence { vec{-3,9,6} });
  }
}
/*
int main() {
  //Sequence a = vec{2,3,vec{4,5}};
  //auto lengths = get_lengths(a);
  //for (auto& x : lengths) std::cout << x << ' '; std::cout << '\n';

  // [2,3]
  //std::vector<std::vector<my_var>> root1 = {{2,3}};
  // [2,[4],3]
  //std::vector<std::vector<my_var>> root2 = { {2,3} };
  // [2,3]
  //std::vector<std::vector<my_var>> root = {{2,3}};
  //my_pointer mm ({2,6}); 
  struct my_wrap;
  using vvar = std::variant<int, my_wrap>;
  struct my_wrap { vvar data;};
  std::vector<std::vector<int,my_wrap>> root;
  root.push_back(std::vector<int,my_wrap>{});
  root[0].push_back(2);
  root[0].push_back(std::vector<std::variant<int, my_wrap>>

}
*/

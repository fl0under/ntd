#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <typeinfo>
#include <type_traits>
#include <variant>
#include "prettyprint.hpp"

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
}
using impl::Sequence;
using impl::vec;

// Recursive visit to hide access to data
template <typename Visitor, typename Variant>
decltype(auto) visit_recursively(Visitor&& visitor, Variant variant) {
  return std::visit (std::forward<Visitor>(visitor),
                     std::forward<Variant>(variant).data);
}

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
  auto gen = [&](const auto& v) {
    // The iterator version is prone to accessing invalid memory
    // if other vectors in the parent Sequence are resized after
    // it is initilised.
    /*
    static auto it = v.begin();
    it = (it != v.end()) ? it : v.begin();
    return *(it++);
    */
    static int i{0};
    i = ((i+1) != v.size()) ? i : 0;
    return v.at(i);
  };

  auto norm = [&](auto& v, int n) {
    auto g = std::bind(gen, v);
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
T normalise(T &e, std::vector<int> s) {
  //auto len = e[s[0]].size();
  //if (e[1].size() != len)
  //  repeat(e[1], len);
  // Check s is consistent
  // Get the size of elements E_i where i is in s
  std::transform(s.begin(), s.end(), s.begin(),
      [&](int i) -> int { return int(e.at(i).size()); });
  std::cout << s << '\n';
  // Check they are equal
  if (std::all_of(s.begin(), s.end(), [&](int i){ return i == s[0]; }))
    std::cout << "all equal\n";
  //if (!std::all_of(s.cbegin(), s.cend(), [](int i){
  // for i in e where i is not in s, repeat(E_i, L')
  // where L' is the length of elements E_i for i in s.
  // std::replace_if(e.begin(), e.end(),
  //   
  return e;
}

template <typename T>
std::vector<std::vector<T>> repeat(std::vector<T> &e, int n) {
  if (n < 2) return e;
  return std::vector<std::vector<T>> (n, e);
}

Sequence repeat(Sequence &s, int n) {
  if (n < 2) return s;
  return Sequence {vec (n, s)};
}

// Minimum
template <typename T>
int minimum(T &a) {
  return size(a);
}

template <typename T, typename... Args>
int minimum(T &a, Args... args) {
  return std::min(int(size(a)), minimum(args...));
}

template <typename T>
int order(T &v) {
  return 0;
}

template <typename T>
int order(std::vector<T> &v) {
  return 1 + order(v[0]);
}

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

void norme(Sequence& a, Sequence& b, int level) {
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
      norme(std::get<vec>(a).at(i).data, std::get<vec>(b).at(i).data, level-1);
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
  // move to top
  int max_order{0};
  if (order(a) > order(b))
    max_order = order(a);
  else
    max_order = order(b);
  norme(a, b, max_order);
}

void norme(Sequence& a, Sequence& b) {
  int max_order{0};
  if (order(a) > order(b))
    max_order = order(a);
  else
    max_order = order(b);
  norme(a, b, max_order);
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
}

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
  auto gen = [=](const auto& v) {
    static auto it = v.begin();
    it = (it != v.end()) ? it : v.begin();
    return *(it++);
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
  std::cout << "any";
  return 0;
}

template <typename T>
int order(std::vector<T> &v) {
  std::cout << "vector";
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

// Distribute


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

  print(c);
  print(d);
  print(e);
  print(f);
  print(g);

  //std::cout << "order of c: " << order(c) << '\n';
  //std::cout << "order of var1: " << order(var1) << '\n';
  std::cout << "order of d: " << order(d) << '\n';
  std::cout << "order of e: " << order(e) << '\n';
  std::cout << "order of f: " << order(f) << '\n';

  print(repeat(d, 3));

  std::cout << "Test generator\n";
  normalise(std::get<vec>(d), std::get<vec>(g));
  print(d);
  print(g);

  //std::cout << "vec1: " << vec1 << '\n';
  //std::cout << "vec2: " << vec2 << '\n';
  //std::cout << "vec3: " << vec3 << '\n';
  //std::cout << "vec4: " << vec4 << '\n';
  //std::cout << "Level of vec3 " << order(vec3) << '\n';
  //std::cout << "Level of vec4 " << order(vec4) << '\n';
  //std::cout << "Level of num " << order(num) << '\n';

  //auto res = repeat(vec3, 2);
  //std::cout << "result: " << res << '\n';
  
  //normalise(vec5, std::vector<int> {1,0});
  //normalise(vec4, std::vector<int> {1,0});


  //std::cout << minimum(vec1, vec3) << '\n';

  //normalise<> (vec1, vec2);
  //normalise<> (vec3, vec4);

  //std::cout << "vec1: " << vec1 << '\n';
  //std::cout << "vec2: " << vec2 << '\n';
  //std::cout << "vec3: " << vec3 << '\n';
  //std::cout << "vec4: " << vec4 << '\n';
  
  //int num1 =1;
  //int num2=4;
  //normalise<> (num1,num2);
}

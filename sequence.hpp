#include <vector>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <typeinfo>
#include <type_traits>
#include <variant>
#include <initializer_list>
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
  using Raw_Sequence = std::variant<int, vec>;

  struct wrapper {
    Raw_Sequence data;

    template <typename... Ts>
    wrapper(Ts&&... xs)
      : data{std::forward<Ts>(xs)...} {}
  };

  Raw_Sequence operator+ (Raw_Sequence lhs, Raw_Sequence rhs) {
    return std::visit( [](auto const &l, auto const &r) -> Raw_Sequence {
        return l + r;
    }, lhs, rhs);
  }

  Raw_Sequence operator* (Raw_Sequence lhs, Raw_Sequence rhs) {
    return std::visit( [](auto const &l, auto const &r) -> Raw_Sequence {
        return l * r;
    }, lhs, rhs);
  }

  bool operator== (Raw_Sequence lhs, Raw_Sequence rhs) {
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

  bool operator!= (Raw_Sequence lhs, Raw_Sequence rhs) {
    return !(lhs == rhs);
  }

  // Enable printing of a Raw_Sequence
  std::ostream &operator<< (std::ostream &os, Raw_Sequence s) {
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
using impl::Raw_Sequence;
using impl::vec;

// Normalised Raw_Sequence data structure
struct Sequence {
  std::vector<int> data;
  std::vector<int> lengths;
  Sequence(std::vector<int> d, std::vector<int> l)
    : data{d}, lengths{l} {}
};

// NTD: Normalise Transpose Distribute

// Normalise the length of two containers by repeating elements
// of the smaller container.
template <typename T>
constexpr void repeat_elements(T &a, int b) {
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


void clone_elements(Raw_Sequence &s, int n) {
  if (n < 2) return;
  s = Raw_Sequence {vec (n, s)};
}

// Get the max length at each level/depth
void get_length(std::vector<int>& lengths, int order, const Raw_Sequence s) {
  if (std::holds_alternative<int>(s)) return;
  if (order > lengths.size()) lengths.push_back(0);
  int n = std::get<vec>(s).size();
  if (n > lengths.at(order-1))
    lengths.at(order-1) = n;
  for (auto& x : std::get<vec>(s))
    get_length(lengths, order+1, x.data);
}

std::vector<int> get_lengths(const Raw_Sequence s) {
  std::vector<int> lengths {0};
  get_length(lengths, 1, s);
  return lengths;
}

std::vector<int> get_lengths(std::initializer_list<Raw_Sequence> l) {
  std::vector<std::vector<int>> all_lengths {};

  // Get the length vectors of each Raw_Sequence in the list and store them
  std::transform(l.begin(), l.end(), std::back_inserter(all_lengths),
      [](const Raw_Sequence s) -> std::vector<int> { return get_lengths(s); });

  // Find the longest length vector
  auto max_length_it = std::max_element(all_lengths.begin(), all_lengths.end(),
      [](std::vector<int>& a, std::vector<int>& b) -> bool { 
        return (a.size() < b.size()); 
      });
  int max_length = (*max_length_it).size();
  
  std::vector<int> lengths (max_length);

  // Find the longest Raw_Sequence at each level
  for (const auto& v : all_lengths) {
    for (int i{0}; i < v.size(); ++i) {
      if (v.at(i) > lengths.at(i))
        lengths.at(i) = v.at(i);
    }
  }
  return lengths;
}


void copy_elements(
    std::vector<int>& norm_s, const std::vector<int>& lengths, int order, Raw_Sequence s, int& start_pos) {

  bool done {false};
  int n{0};

  if (order > lengths.size()) {
    // Must have reached a terminal vector
    done = true;
    n = 1;
  } else {
    // Make the current element the right length
    // For a vector, repeat elements until have the required length
    if (std::holds_alternative<vec>(s))
      repeat_elements(std::get<vec>(s), lengths.at(order-1));
    // For a number, just clone it to get required length
    else {
      n = std::accumulate( lengths.begin() + order-1,  lengths.end(),
          1, std::multiplies<int>() );
      clone_elements(s, n);
      done = true;
    }
  }

  if (done) {
    // Copy it across to the final vector
    if (std::holds_alternative<vec>(s)) {
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

Sequence normalise(Raw_Sequence s, std::vector<int> lengths) {
  std::vector<int> norm_s ( std::accumulate(
        lengths.begin(), lengths.end(), 1, std::multiplies<int>()) );
  int start_pos {0};
  copy_elements(norm_s, lengths, 1, s, start_pos);
  return Sequence(norm_s, lengths);
}


namespace sequence_operator {
  Raw_Sequence plus(impl::wrapper& a, impl::wrapper& b) {
    return a.data + b.data;
  }

  Raw_Sequence multiplies(impl::wrapper& a, impl::wrapper& b) {
    return a.data * b.data;
  }
}

// Pass functions using template params for now,
// maybe change to function_view from here later.
// https://vittorioromeo.info/index/blog/passing_functions_to_functions.htm
template <typename TF>
Sequence transpose_distribute(Raw_Sequence a, Raw_Sequence b, TF&& func) {
  // normalise
  auto lengths = get_lengths({a,b});
  Sequence norm_a = normalise(a, lengths);
  Sequence norm_b = normalise(b, lengths);

  std::vector<int> result (norm_a.data.size());

  std::transform(
      norm_a.data.begin(), 
      norm_a.data.end(),
      norm_b.data.begin(),
      result.begin(),
      func
      );

  return Sequence(result, lengths);
}


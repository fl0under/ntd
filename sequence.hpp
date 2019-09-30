#include <vector>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <functional>
#include <variant>
#include <iterator>
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
  Sequence() {}
  Sequence(std::vector<int> d, std::vector<int> l)
    : data{d}, lengths{l} {}
};

// NTD: Normalise Transpose Distribute

// Normalise the length of two containers by repeating elements
// of the smaller container.
template <typename T>
constexpr void repeat_elements(T &a, int final_size) {
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

  int diff = final_size - a.size();

  if (diff <= 0)
    return;
  else if (diff > 0)
    norm(a, diff);
}

// Repeat section of a from begin to end, inserting at position end.
// final_size is final size between begin and end.
template <typename T>
constexpr void repeat_elements(T &a, int final_size, int begin, int end) {
  auto gen = [&](const auto& v, int& i, int begin, int end) {
    if (i == end) i = begin;
    return v.at(i++);
  };

  auto norm = [&](auto& v, int n) {
    int index{begin};
    auto g = std::bind(gen, v, index, begin, end);
    v.insert(v.begin()+end, n, 0);
    std::generate(v.begin()+end, v.begin()+end+n, g);
  };

  int diff = final_size - (end-begin);

  if (diff <= 0)
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

std::vector<int> get_lengths(std::initializer_list<Sequence> l) {
  // Find the longest length vector
  auto max_length_it = std::max_element(l.begin(), l.end(),
      [](Sequence a, Sequence b) -> bool { 
        return (a.lengths.size() < b.lengths.size()); 
      });
  int max_length = (*max_length_it).lengths.size();

  std::vector<int> lengths (max_length);

  // Find the longest Raw_Sequence at each level
  for (const auto& v : l) {
    for (int i{0}; i < v.lengths.size(); ++i) {
      if (v.lengths.at(i) > lengths.at(i))
        lengths.at(i) = v.lengths.at(i);
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

Sequence normalise(Sequence s, std::vector<int> lengths) {
  int diff = lengths.size() - s.lengths.size();
  if (diff > 0) {
    s.lengths.insert(s.lengths.begin(), diff, 1);
  }

  for (int order{int(lengths.size()-1)}; order >= 0; --order) {
    const int n = s.data.size();
    int old_section_length = std::accumulate( 
      s.lengths.begin() + order, s.lengths.end(), 1, std::multiplies<int>() );
    int new_section_length = std::accumulate( 
      lengths.begin() + order, lengths.end(), 1, std::multiplies<int>() );
    int begin{0}, end{old_section_length};

    if (s.lengths.at(order) < lengths.at(order)) {
      for (int i{0}; i < n / old_section_length; ++i) {
        repeat_elements(s.data, new_section_length, begin, end);
        begin += new_section_length;
        end += new_section_length;
      }
      s.lengths.at(order) = lengths.at(order);
    }
  }
  return s;
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

/* Functions.
 * For example a function with this signature: my_func := (scalar x, vector y)
 * will be like
 * hash_map_order[my_func] = [0,1]
 *
 */

/*
template <typename T, typename... Args>
int minimum(T &a, Args... args) {
  return std::min(int(size(a)), minimum(args...));
}

template <typename T, typename... Args>
int plus(T &a, Args... args) {
  return std::plus<T>( a,plus(args...) );
}

template <typename TF>
Sequence transpose_distribute(std::initializer_list<Raw_Sequence> l, TF&& func) {
  // normalise
  auto lengths = get_lengths(l);
  std::array<Sequence, l.size()> seqs;
  std::transform(
      l.begin(),
      l.end(),
      seqs.begin(),
      [&](Raw_Sequence s) { return normalise(s, lengths); }
      );

  std::vector<int> result (seqs[0].data.size());

  // transpose distribute
  // *(l.begin()+1)

  // Transpose column
  std::
  
  std::transform(
      norm_a.data.begin(), 
      norm_a.data.end(),
      norm_b.data.begin(),
      result.begin(),
      plus(
      );

  return Sequence(result, lengths);
}
*/

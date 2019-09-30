//#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT

#include "doctest.h"
#include "sequence.hpp"

TEST_CASE("repeating elements") {
  std::vector<int> a;

  SUBCASE("whole vector") {
    a = {1,2,3,4,5};
    repeat_elements(a, 8, 0, a.size() );
    CHECK(a == std::vector<int>{1,2,3,4,5,1,2,3});
  }

  SUBCASE("part of vector") {
    a = {1,2,3,4,5};
    repeat_elements(a, 5, 1, a.size()-2 );
    CHECK(a == std::vector<int>{1,2,3,2,3,2,4,5});
  }
}

TEST_CASE("getting lengths") {
  Raw_Sequence a,b,c,d;

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

  SUBCASE("multiple lengths") {
    a = vec{2,3,4};
    b = vec{3,vec{2,4}};
    c = vec{7};
    auto lengths = get_lengths({a,b,c});
    CHECK(lengths == std::vector<int>{3,2});
  }

  SUBCASE("multiple lengths") {
    a = vec{ vec{2,7,8}, vec{4,8} };
    b = 6;
    c = vec{ vec{5}, vec{3,6,9}, vec{2,2} };
    auto lengths = get_lengths({a,b,c});
    CHECK(lengths == std::vector<int>{3,3});
  }

  SUBCASE("multiple lengths") {
    a = vec{2,3,4,6,7,8,3};
    b = vec{3,vec{2,4}};
    c = vec{7};
    d = vec{vec{vec{2,8,4}}};
    auto lengths = get_lengths({a,b,d,c});
    CHECK(lengths == std::vector<int>{7,2,3});
  }
}

TEST_CASE("Testing get lengths for Sequence") {
  Sequence a,b,c,d;

  SUBCASE("order 1") {
    a.lengths = std::vector<int> {2};
    b.lengths = std::vector<int> {4};
    auto lengths = get_lengths({a,b});
    CHECK(lengths == std::vector<int>{4});
  }

  SUBCASE("order 2") {
    a.lengths = std::vector<int> {2,5};
    b.lengths = std::vector<int> {4,3};
    auto lengths = get_lengths({a,b});
    CHECK(lengths == std::vector<int>{4,5});
  }
}

TEST_CASE("normalise") {
  Raw_Sequence a;

  SUBCASE("order 1") {
    a = vec{2,3,4};
    auto lengths = get_lengths(a);
    auto normalised = normalise(a, lengths);
    CHECK(normalised.data == std::vector<int>{2,3,4});
    CHECK(normalised.lengths == std::vector<int>{3});
  }

  SUBCASE("order 2") {
    a = vec{2,3,vec{7,8},4};
    auto lengths = get_lengths(a);
    auto normalised = normalise(a, lengths);
    CHECK(normalised.data == std::vector<int>{2,2,3,3,7,8,4,4});
    CHECK(normalised.lengths == std::vector<int>{4,2});
  }

  SUBCASE("order 3") {
    a = vec{vec{6,9,3},3,vec{7,8},4};
    auto lengths = get_lengths(a);
    auto normalised = normalise(a, lengths);
    CHECK(normalised.data == std::vector<int>{6,9,3,3,3,3,7,8,7,4,4,4});
    CHECK(normalised.lengths == std::vector<int>{4,3});
  }
}


TEST_CASE("testing normalise, multiple Raw_Sequences") {
  Raw_Sequence a,b,c;

  SUBCASE("order 1") {
    a = vec{3,4};
    b = vec{7,5,8};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector<int>{3,4,3});
    CHECK(norm_b.data == std::vector<int>{7,5,8});
    CHECK(norm_a.lengths == std::vector<int>{3});
    CHECK(norm_b.lengths == std::vector<int>{3});
  }

  SUBCASE("order 1") {
    a = vec{3,vec{5,6},4};
    b = vec{7,5,8,1};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);
    CHECK(norm_a.data == std::vector<int>{3,3,5,6,4,4,3,3});
    CHECK(norm_b.data == std::vector<int>{7,7,5,5,8,8,1,1});
    CHECK(norm_a.lengths == std::vector<int>{4,2});
    CHECK(norm_b.lengths == std::vector<int>{4,2});
  }

  SUBCASE("order 1, same length") {
    a = vec{2,3,4};
    b = vec{3,2,6};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector<int>{2,3,4} );
    CHECK(norm_b.data == std::vector<int>{3,2,6} );
    CHECK(norm_a.lengths == std::vector<int>{3});
    CHECK(norm_b.lengths == std::vector<int>{3});
  }

  SUBCASE("order 2, same sequence") {
    a = vec{vec{3,2},1};
    auto lengths = get_lengths(a);
    auto norm_a = normalise(a, lengths);

    CHECK(norm_a.data == std::vector{3,2,1,1} );
    CHECK(norm_a.lengths == std::vector<int>{2,2});
  }
  
  SUBCASE("order 1") {
    a = vec{1,2,3,4,5};
    b = vec{6,7};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector<int>{1,2,3,4,5} );
    CHECK(norm_b.data == std::vector<int>{6,7,6,7,6} );
    CHECK(norm_a.lengths == std::vector<int>{5});
    CHECK(norm_b.lengths == std::vector<int>{5});
  }

  SUBCASE("order 2") {
    a = vec{vec{2,3},vec{5,7}};
    b = vec{vec{8,9},vec{2,1},vec{7,6}};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector{ 2,3,5,7,2,3 });
    CHECK(norm_b.data == std::vector{ 8,9,2,1,7,6 });
    CHECK(norm_a.lengths == std::vector<int>{3,2});
    CHECK(norm_b.lengths == std::vector<int>{3,2});
  }

  SUBCASE("order 3") {
    a = vec{vec{vec{2,2},vec{3,3}}, vec{vec{5,5},vec{7,7}}};
    b = vec{vec{vec{8,8},vec{9,9}}, vec{vec{2,2},vec{1,1}}, vec{vec{7,7},vec{6,6}}};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector{ 2,2,3,3,5,5,7,7,2,2,3,3 });
    CHECK(norm_b.data == std::vector{ 8,8,9,9,2,2,1,1,7,7,6,6 });
    CHECK(norm_a.lengths == std::vector<int>{3,2,2});
    CHECK(norm_b.lengths == std::vector<int>{3,2,2});
  }

  SUBCASE("order delta 1, different length") {
    a = vec{6, 7};
    b = vec{8, vec{3, 4}, 1};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector{ 6,6,7,7,6,6 });
    CHECK(norm_b.data == std::vector{ 8,8,3,4,1,1 });
    CHECK(norm_a.lengths == std::vector<int>{3,2});
    CHECK(norm_b.lengths == std::vector<int>{3,2});
  }

  SUBCASE("order delta 1, different length") {
    a = vec{ vec{1,2,3}, vec{4,5,6}, vec{7,8,9} };
    b = vec{ vec{2,4,5} };
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector{ 1,2,3,4,5,6,7,8,9 });
    CHECK(norm_b.data == std::vector{ 2,4,5,2,4,5,2,4,5 });
    CHECK(norm_a.lengths == std::vector<int>{3,3});
    CHECK(norm_b.lengths == std::vector<int>{3,3});
  }

  SUBCASE("order delta 1, same length") {
    a = vec{1,2,3};
    b = vec{5, vec{3,4}, vec{7,6}};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector{ 1,1,2,2,3,3 });
    CHECK(norm_b.data == std::vector{ 5,5,3,4,7,6 });
    CHECK(norm_a.lengths == std::vector<int>{3,2});
    CHECK(norm_b.lengths == std::vector<int>{3,2});
  }

  SUBCASE("order delta 2, same length") {
    a = vec{1,2,3};
    b = vec{5, vec{vec{3,0},4}, vec{7,6}};
    auto lengths = get_lengths({a,b});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);

    CHECK(norm_a.data == std::vector{ 1,1, 1,1,
                                      2,2, 2,2,
                                      3,3, 3,3 }
                        );
    CHECK(norm_b.data == std::vector{ 5,5, 5,5,
                                      3,0, 4,4,
                                      7,7, 6,6 }
                        );
    CHECK(norm_a.lengths == std::vector<int>{3,2,2});
    CHECK(norm_b.lengths == std::vector<int>{3,2,2});
  }

  // Example from "SequenceL provides a different way to view programming"
  SUBCASE("order delta 2, different length") {
    a = vec{ vec{2,7,8}, vec{4,8} };
    b = 6;
    c = vec{ vec{5}, vec{3,6,9}, vec{2,2} };
    auto lengths = get_lengths({a,b,c});
    auto norm_a = normalise(a, lengths);
    auto norm_b = normalise(b, lengths);
    auto norm_c = normalise(c, lengths);

    CHECK(norm_a.data == std::vector{ 2,7,8, 4,8,4, 2,7,8 });
    CHECK(norm_b.data == std::vector{ 6,6,6, 6,6,6, 6,6,6 });
    CHECK(norm_c.data == std::vector{ 5,5,5, 3,6,9, 2,2,2 });
    CHECK(norm_a.lengths == std::vector<int>{3,3});
    CHECK(norm_b.lengths == std::vector<int>{3,3});
    CHECK(norm_c.lengths == std::vector<int>{3,3});
  }
}

TEST_CASE("normalise Sequence") {
  Sequence a;

  SUBCASE("order 1") {
    a.data = {2,3,4};
    a.lengths = {3};
    auto normalised = normalise(a, std::vector<int>{5});
    CHECK(normalised.data == std::vector<int>{2,3,4,2,3});
    CHECK(normalised.lengths == std::vector<int>{5});
  }

  SUBCASE("order 2") {
    a.data = {2,2,3,3,7,8,4,4};
    a.lengths = {4,2};
    auto normalised = normalise(a, std::vector<int>{4,3});
    CHECK(normalised.data == std::vector<int>{2,2,2,3,3,3,7,8,7,4,4,4});
    CHECK(normalised.lengths == std::vector<int>{4,3});
  }

  SUBCASE("order 2") {
    a.data = {2,2,3,3,7,8,4,4};
    a.lengths = {4,2};
    auto normalised = normalise(a, std::vector<int>{5,3});
    CHECK(normalised.data == std::vector<int>{2,2,2,3,3,3,7,8,7,4,4,4,2,2,2});
    CHECK(normalised.lengths == std::vector<int>{5,3});
  }

  SUBCASE("order 1 plus one") {
    a.data = {2,3,4};
    a.lengths = {3};
    auto normalised = normalise(a, std::vector<int>{2,3});
    CHECK(normalised.data == std::vector<int>{2,3,4,2,3,4});
    CHECK(normalised.lengths == std::vector<int>{2,3});
  }

  SUBCASE("order 3") {
    a.data = {4,5,6,4,5,6};
    a.lengths = {2,3,1};
    auto normalised = normalise(a, std::vector<int>{2,3,2});
    CHECK(normalised.data == std::vector<int>{4,4,5,5,6,6,4,4,5,5,6,6});
    CHECK(normalised.lengths == std::vector<int>{2,3,2});
  }

  SUBCASE("order 1, ignore normalise to smaller size") {
    a.data = {4,5,6};
    a.lengths = {3};
    auto normalised = normalise(a, std::vector<int>{2});
    CHECK(normalised.data == std::vector<int>{4,5,6});
    CHECK(normalised.lengths == std::vector<int>{3});
  }
}

TEST_CASE("testing transpose-distribute") {
  Raw_Sequence a,b;

  SUBCASE("order 1, plus") {
    a = vec{2,3,4};
    b = vec{3,2,6};
    auto result = transpose_distribute(a,b, std::plus<int>());

    CHECK(result.data == std::vector{ 5,5,10 });
  }

  SUBCASE("order 1, multiply") {
    a = vec{2,3,4};
    b = vec{3,2,6};
    auto result = transpose_distribute(a,b, std::multiplies<int>());

    CHECK(result.data == std::vector{ 6,6,24 });
  }

  SUBCASE("order 2, multiply") {
    a = vec{vec{1,2},vec{3,4}};
    b = vec{vec{5,6},vec{7,8}};
    auto result = transpose_distribute(a, b, std::multiplies<int>());

    CHECK(result.data == std::vector{ 5,12,21,32 });
  }

  SUBCASE("order 1, multiply") {
    a = vec{3,3,3};
    b = vec{-1,3,2};
    auto result = transpose_distribute(a, b, std::multiplies<int>());

    CHECK(result.data == std::vector{ -3,9,6 });
  }

  SUBCASE("order delta 1, multiply") {
    a = vec{4,5,6,7};
    b = 10;
    auto result = transpose_distribute(a, b, std::multiplies<int>());

    CHECK(result.data == std::vector{ 40,50,60,70 });
  }

  SUBCASE("order delta 2, multiply") {
    a = vec{ vec{2,3}, vec{4,6} };
    b = 10;
    auto result = transpose_distribute(a, b, std::multiplies<int>());

    CHECK(result.data == std::vector{ 20,30,40,60 });
  }

  SUBCASE("order delta 2, multiply") {
    a = vec{ vec{2,3}, vec{4,6,7} };
    b = 10;
    auto result = transpose_distribute(a, b, std::multiplies<int>());

    CHECK(result.data == std::vector{ 20,30,20,40,60,70 });
  }
}

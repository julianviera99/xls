// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/ir/type.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "xls/ir/function.h"
#include "xls/ir/function_builder.h"

namespace xls {
namespace {

TEST(TypeTest, TestVariousTypes) {
  BitsType b42(42);
  BitsType b42_2(42);
  BitsType b123(123);

  EXPECT_TRUE(b42.IsEqualTo(&b42));
  EXPECT_TRUE(b42.IsEqualTo(&b42_2));
  EXPECT_FALSE(b42.IsEqualTo(&b123));

  EXPECT_EQ(b42.leaf_count(), 1);
  EXPECT_EQ(b42_2.leaf_count(), 1);
  EXPECT_EQ(b123.leaf_count(), 1);

  TupleType t_empty({});
  TupleType t1({&b42, &b42});
  TupleType t2({&b42, &b42});
  TupleType t3({&b42, &b42_2});
  TupleType t4({&b42, &b42, &b42});

  EXPECT_TRUE(t_empty.IsEqualTo(&t_empty));
  EXPECT_FALSE(t_empty.IsEqualTo(&t1));
  EXPECT_FALSE(t_empty.IsEqualTo(&b42));
  EXPECT_TRUE(t1.IsEqualTo(&t1));
  EXPECT_TRUE(t1.IsEqualTo(&t2));
  EXPECT_TRUE(t1.IsEqualTo(&t3));
  EXPECT_FALSE(t1.IsEqualTo(&t4));

  EXPECT_EQ(t_empty.leaf_count(), 0);
  EXPECT_EQ(t4.leaf_count(), 3);

  TupleType t_nested_empty({&t_empty});
  TupleType t_nested1({&t1, &t2});
  TupleType t_nested2({&t2, &t1});
  TupleType t_nested3({&t1, &t3});
  TupleType t_nested4({&t1, &t4});

  EXPECT_TRUE(t_nested_empty.IsEqualTo(&t_nested_empty));
  EXPECT_FALSE(t_nested_empty.IsEqualTo(&t_empty));
  EXPECT_TRUE(t_nested1.IsEqualTo(&t_nested2));
  EXPECT_TRUE(t_nested1.IsEqualTo(&t_nested3));
  EXPECT_FALSE(t_nested1.IsEqualTo(&t_nested4));

  EXPECT_EQ(t_nested_empty.leaf_count(), 0);
  EXPECT_EQ(t_nested3.leaf_count(), 4);

  ArrayType a1(7, &b42);
  ArrayType a2(7, &b42_2);
  ArrayType a3(3, &b42);
  ArrayType a4(7, &b123);

  EXPECT_TRUE(a1.IsEqualTo(&a1));
  EXPECT_TRUE(a1.IsEqualTo(&a2));
  EXPECT_FALSE(a1.IsEqualTo(&a3));
  EXPECT_FALSE(a1.IsEqualTo(&a4));

  EXPECT_EQ(a1.leaf_count(), 7);
  EXPECT_EQ(a3.leaf_count(), 3);

  // Arrays-of-tuples.
  ArrayType a_of_t1(42, &t1);
  ArrayType a_of_t2(42, &t2);
  ArrayType a_of_t3(42, &t4);

  EXPECT_TRUE(a_of_t1.IsEqualTo(&a_of_t2));
  EXPECT_FALSE(a_of_t1.IsEqualTo(&a_of_t3));

  EXPECT_EQ(a_of_t1.leaf_count(), 84);
  EXPECT_EQ(a_of_t3.leaf_count(), 126);

  // Tuple-of-Arrays.
  TupleType t_of_a1({&a1, &a1, &a2});
  TupleType t_of_a2({&a1, &a1, &a1});
  TupleType t_of_a3({&a1, &a2, &a3});

  EXPECT_TRUE(t_of_a1.IsEqualTo(&t_of_a2));
  EXPECT_FALSE(t_of_a1.IsEqualTo(&t_of_a3));
  EXPECT_FALSE(t_of_a1.IsEqualTo(&b42));

  EXPECT_EQ(t_of_a1.leaf_count(), 21);
  EXPECT_EQ(t_of_a3.leaf_count(), 17);

  // Function types.
  FunctionType f_type1({&b42, &a1}, &b42);
  FunctionType f_type2({&b42, &a2}, &b42);
  FunctionType f_type3({&b42}, &b42);
  FunctionType f_type4({}, &b42);
  FunctionType f_type5({&b42, &a1}, &b123);

  EXPECT_TRUE(f_type1.IsEqualTo(&f_type2));
  EXPECT_FALSE(f_type1.IsEqualTo(&f_type3));
  EXPECT_FALSE(f_type1.IsEqualTo(&f_type4));
  EXPECT_FALSE(f_type1.IsEqualTo(&f_type5));
}

}  // namespace
}  // namespace xls

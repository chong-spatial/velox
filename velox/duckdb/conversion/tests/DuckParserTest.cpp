/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "velox/duckdb/conversion/DuckParser.h"
#include <gtest/gtest.h>
#include "velox/parse/Expressions.h"

using namespace facebook::velox;
using namespace facebook::velox::duckdb;

TEST(DuckParserTest, constants) {
  // Integers.
  EXPECT_EQ("0", parseExpr("0")->toString());
  EXPECT_EQ("100", parseExpr("100")->toString());
  EXPECT_EQ("-787", parseExpr("-787")->toString());
  EXPECT_EQ(
      "9223372036854775807", parseExpr("9223372036854775807")->toString());

  // Doubles.
  EXPECT_EQ("1.99", parseExpr("1.99")->toString());
  EXPECT_EQ("-303.1234", parseExpr("-303.1234")->toString());

  // Strings.
  EXPECT_EQ("\"\"", parseExpr("''")->toString());
  EXPECT_EQ("\"hello world\"", parseExpr("'hello world'")->toString());

  // Nulls
  EXPECT_EQ("null", parseExpr("NULL")->toString());
}

TEST(DuckParserTest, arrays) {
  // Literal arrays with different types.
  EXPECT_EQ("[1,2,-33]", parseExpr("ARRAY[1, 2, -33]")->toString());
  EXPECT_EQ(
      "[1.99,-8.3,0.878]", parseExpr("ARRAY[1.99, -8.3, 0.878]")->toString());
  EXPECT_EQ(
      "[\"asd\",\"qwe\",\"ewqq\"]",
      parseExpr("ARRAY['asd', 'qwe', 'ewqq']")->toString());
  EXPECT_EQ("[null,1]", parseExpr("ARRAY[NULL, 1]")->toString());

  // Empty array.
  EXPECT_EQ("[]", parseExpr("ARRAY[]")->toString());

  // Only simple constants are supported as part of array literals.
  EXPECT_THROW(parseExpr("ARRAY[1 + 2]"), VeloxUserError);
}

TEST(DuckParserTest, variables) {
  // Variables.
  EXPECT_EQ("\"c1\"", parseExpr("c1")->toString());
  EXPECT_EQ("\"c1\"", parseExpr("\"c1\"")->toString());
  EXPECT_EQ("dot(\"tbl\",\"c2\")", parseExpr("tbl.c2")->toString());
}

TEST(DuckParserTest, functions) {
  EXPECT_EQ("avg(\"col1\")", parseExpr("avg(col1)")->toString());
  EXPECT_EQ(
      "func(1,3.4,\"str\")", parseExpr("func(1, 3.4, 'str')")->toString());

  // Nested calls.
  EXPECT_EQ(
      "f(100,g(h(3.6)),99)", parseExpr("f(100, g(h(3.6)), 99)")->toString());
}

TEST(DuckParserTest, subscript) {
  EXPECT_EQ("subscript(\"c\",0)", parseExpr("c[0]")->toString());
  EXPECT_EQ(
      "subscript(\"col\",plus(10,99))", parseExpr("col[10 + 99]")->toString());
  EXPECT_EQ("subscript(\"m1\",34)", parseExpr("m1[34]")->toString());
  EXPECT_EQ(
      "subscript(\"m2\",\"key str\")", parseExpr("m2['key str']")->toString());

  EXPECT_EQ(
      "subscript(func(),\"key str\")",
      parseExpr("func()['key str']")->toString());
}

TEST(DuckParserTest, coalesce) {
  EXPECT_EQ("coalesce(null,0)", parseExpr("coalesce(NULL, 0)")->toString());
  EXPECT_EQ(
      "coalesce(\"col1\",plus(0,3),\"col2\",1002)",
      parseExpr("coalesce(col1,  0+ 3, col2, 1002)")->toString());
}

TEST(DuckParserTest, in) {
  EXPECT_EQ("in(\"col1\",1,2,3)", parseExpr("col1 in (1, 2, 3)")->toString());
  EXPECT_EQ(
      "in(\"col1\",1,2,null,3)",
      parseExpr("col1 in (1, 2, null, 3)")->toString());
  EXPECT_EQ(
      "in(\"col1\",\"a\",\"b\",\"c\")",
      parseExpr("col1 in ('a', 'b', 'c')")->toString());
  EXPECT_EQ(
      "in(\"col1\",\"a\",null,\"b\",\"c\")",
      parseExpr("col1 in ('a', null, 'b', 'c')")->toString());
}

TEST(DuckParserTest, expressions) {
  // Comparisons.
  EXPECT_EQ("eq(1,0)", parseExpr("1 = 0")->toString());
  EXPECT_EQ("neq(1,0)", parseExpr("1 != 0")->toString());
  EXPECT_EQ("neq(1,0)", parseExpr("1 <> 0")->toString());
  EXPECT_EQ("not(1)", parseExpr("!1")->toString());
  EXPECT_EQ("gt(1,0)", parseExpr("1 > 0")->toString());
  EXPECT_EQ("gte(1,0)", parseExpr("1 >= 0")->toString());
  EXPECT_EQ("lt(1,0)", parseExpr("1 < 0")->toString());
  EXPECT_EQ("lte(1,0)", parseExpr("1 <= 0")->toString());

  // Arithmetic operators.
  EXPECT_EQ("plus(1,0)", parseExpr("1 + 0")->toString());
  EXPECT_EQ("minus(1,0)", parseExpr("1 - 0")->toString());
  EXPECT_EQ("multiply(1,0)", parseExpr("1 * 0")->toString());
  EXPECT_EQ("divide(1,0)", parseExpr("1 / 0")->toString());
  EXPECT_EQ("modulus(1,0)", parseExpr("1 % 0")->toString());

  // ANDs and ORs.
  EXPECT_EQ("and(1,0)", parseExpr("1 and 0")->toString());
  EXPECT_EQ("or(1,0)", parseExpr("1 or 0")->toString());

  EXPECT_EQ(
      "and(and(and(1,0),2),3)", parseExpr("1 and 0 and 2 and 3")->toString());
  EXPECT_EQ(
      "or(and(1,0),and(2,3))", parseExpr("1 and 0 or 2 and 3")->toString());

  // NOT.
  EXPECT_EQ("not(1)", parseExpr("not 1")->toString());

  // Mix-and-match.
  EXPECT_EQ(
      "gte(plus(modulus(\"c0\",10),0),multiply(f(100),9))",
      parseExpr("c0 % 10 + 0 >= f(100) * 9")->toString());
}

TEST(DuckParserTest, between) {
  EXPECT_EQ(
      "and(gte(\"c0\",0),lte(\"c0\",1))",
      parseExpr("c0 between 0 and 1")->toString());

  EXPECT_EQ(
      "and(and(gte(\"c0\",0),lte(\"c0\",1)),gt(\"c0\",10))",
      parseExpr("c0 between 0 and 1 and c0 > 10")->toString());
}

TEST(DuckParserTest, cast) {
  EXPECT_EQ(
      "cast(\"1\", BIGINT)", parseExpr("cast('1' as bigint)")->toString());
  EXPECT_EQ(
      "cast(0.99, INTEGER)", parseExpr("cast(0.99 as integer)")->toString());
  EXPECT_EQ(
      "cast(0.99, SMALLINT)", parseExpr("cast(0.99 as smallint)")->toString());
  EXPECT_EQ(
      "cast(0.99, TINYINT)", parseExpr("cast(0.99 as tinyint)")->toString());
  EXPECT_EQ("cast(1, DOUBLE)", parseExpr("cast(1 as double)")->toString());
  EXPECT_EQ(
      "cast(\"col1\", REAL)", parseExpr("cast(col1 as real)")->toString());
  EXPECT_EQ(
      "cast(\"col1\", REAL)", parseExpr("cast(col1 as float)")->toString());
  EXPECT_EQ(
      "cast(0.99, VARCHAR)", parseExpr("cast(0.99 as string)")->toString());
  EXPECT_EQ(
      "cast(0.99, VARCHAR)", parseExpr("cast(0.99 as varchar)")->toString());
  EXPECT_EQ(
      "cast(\"str_col\", TIMESTAMP)",
      parseExpr("cast(str_col as timestamp)")->toString());

  // NB: DuckDB returns TIMESTAMP for `cast as date`.
  EXPECT_EQ(
      "cast(\"str_col\", TIMESTAMP)",
      parseExpr("cast(str_col as date)")->toString());

  // Unsupported casts for now.
  EXPECT_THROW(parseExpr("cast('2020-01-01' as TIME)"), std::runtime_error);
}

TEST(DuckParserTest, ifCase) {
  EXPECT_EQ("if(99,1,0)", parseExpr("if(99, 1, 0)")->toString());
  EXPECT_EQ(
      "if(\"a\",plus(\"b\",\"c\"),g(\"d\"))",
      parseExpr("if(a, b + c, g(d))")->toString());

  // CASE statements.
  EXPECT_EQ(
      "if(1,null,0)",
      parseExpr("case when 1 then NULL else 0 end")->toString());
  EXPECT_EQ(
      "if(gt(\"a\",0),1,0)",
      parseExpr("case when a > 0 then 1 else 0 end")->toString());
}

TEST(DuckParserTest, switchCase) {
  EXPECT_EQ(
      "switch(gt(\"a\",0),1,lt(\"a\",0),-1,0)",
      parseExpr("case when a > 0 then 1 when a < 0 then -1 else 0 end")
          ->toString());

  // no "else" clause
  EXPECT_EQ(
      "switch(gt(\"a\",0),1,lt(\"a\",0),-1)",
      parseExpr("case when a > 0 then 1 when a < 0 then -1end")->toString());

  EXPECT_EQ(
      "switch(eq(\"a\",1),\"x\",eq(\"a\",5),\"y\",\"z\")",
      parseExpr("case a when 1 then 'x' when 5 then 'y' else 'z' end")
          ->toString());
}

TEST(DuckParserTest, invalid) {
  // Invalid.
  EXPECT_THROW(parseExpr(""), std::exception);
  EXPECT_THROW(parseExpr(","), std::exception);
  EXPECT_THROW(parseExpr("a +"), std::exception);
  EXPECT_THROW(parseExpr("f(1"), std::exception);

  // Wrong number of expressions.
  EXPECT_THROW(parseExpr("1, 10"), std::invalid_argument);
  EXPECT_THROW(parseExpr("a, 99.8, 'c'"), std::invalid_argument);
}

TEST(DuckParserTest, isNull) {
  EXPECT_EQ("is_null(\"a\")", parseExpr("a IS NULL")->toString());
}

TEST(DuckParserTest, isNotNull) {
  EXPECT_EQ("not(is_null(\"a\"))", parseExpr("a IS NOT NULL")->toString());
}

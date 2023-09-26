/*******************************************************************************
 * Copyright IBM Corp. and others 2023
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "OpCodeTest.hpp"
#include "default_compiler.hpp"
#include <vector>

static const int32_t returnValueForArraycmpGreaterThan = 2;
static const int32_t returnValueForArraycmpLessThan = 1;
static const int32_t returnValueForArraycmpEqual = 0;
/**
 * @brief TestFixture class for arraycmp test
 *
 * @details Used for arraycmp test with the arrays with same data.
 * The parameter is the length parameter for the arraycmp evaluator.
 */
class EhsanArraycmpEqualTest : public TRTest::JitTest, public ::testing::WithParamInterface<int64_t> {};

TEST_P(EhsanArraycmpEqualTest, SameArrayTest) {
    SKIP_ON_ARM(MissingImplementation);
    SKIP_ON_RISCV(MissingImplementation);

    auto length = GetParam();
    char inputTrees[1024] = {0};
    std::snprintf(inputTrees, sizeof(inputTrees),
      "(method return=Int32 args=[Address, Address, Int64]"
      "  (block"
      "    (lreturn"
      "      (arraycmplen address=0 args=[Address, Address]"
      "        (aload parm=0)"
      "        (aload parm=1)"
      "        (lload parm=2)))))"
      );


    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    std::vector<unsigned char> s1(length, 0x5c);
    auto entry_point = compiler.getEntryPoint<int64_t (*)(unsigned char *, unsigned char *, int64_t)>();
    EXPECT_EQ(0, entry_point(&s1[0], &s1[0], (int64_t)length));
}

TEST_P(EhsanArraycmpEqualTest, DiffrentArrayTest) {
    SKIP_ON_ARM(MissingImplementation);
    SKIP_ON_RISCV(MissingImplementation);

    auto length = GetParam();
    char inputTrees[1024] = {0};
    std::snprintf(inputTrees, sizeof(inputTrees),
      "(method return=Int32 args=[Address, Address, Int64]"
      "  (block"
      "    (lreturn"
      "      (arraycmplen address=0 args=[Address, Address]"
      "        (aload parm=0)"
      "        (aload parm=1)"
      "        (lload parm=2)))))"
      );


    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    std::vector<unsigned char> s1(length, 0x5c);
    std::vector<unsigned char> s2(length, 0x5c);
    auto entry_point = compiler.getEntryPoint<int64_t (*)(unsigned char *, unsigned char *, int64_t)>();
    EXPECT_EQ(0, entry_point(&s1[0], &s2[0], (int64_t)length));
}

INSTANTIATE_TEST_CASE_P(EhsanTest, EhsanArraycmpEqualTest, ::testing::Range(static_cast<int64_t>(255), static_cast<int64_t>(256)));

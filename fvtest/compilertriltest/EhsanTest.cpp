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
class EhsanArraycmpEqualTest : public TRTest::JitTest, public ::testing::WithParamInterface<int32_t> {};

TEST_P(EhsanArraycmpEqualTest, SillyTest) {
    SKIP_ON_ARM(MissingImplementation);
    SKIP_ON_RISCV(MissingImplementation);

    auto length = GetParam();
    char inputTrees[1024] = {0};
    std::snprintf(inputTrees, sizeof(inputTrees),
      "(method return=Int32 args=[Address, Address, Int32]"
      "  (block"
      "    (ireturn"
      "      (arraycmp address=0 args=[Address, Address]"
      "        (aload parm=0)"
      "        (aload parm=1)"
      "        (iload parm=2)))))"
      );
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    std::vector<unsigned char> s1(length, 0x5c);
    std::vector<unsigned char> s2(length, 0x5c);
    auto entry_point = compiler.getEntryPoint<int32_t (*)(unsigned char *, unsigned char *, int32_t)>();
    EXPECT_EQ(returnValueForArraycmpEqual, entry_point(&s1[0], &s2[0], length));
}

INSTANTIATE_TEST_CASE_P(EhsanTest, EhsanArraycmpEqualTest, ::testing::Range(127, 128));

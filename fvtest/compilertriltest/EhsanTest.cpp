/*******************************************************************************
 * Copyright IBM Corp. and others 2025
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

#include "JitTest.hpp"
#include "default_compiler.hpp"
#include "compilerunittest/CompilerUnitTest.hpp"

class VectorTest : public TRTest::JitTest {};

class ByteTest : public VectorTest, public ::testing::WithParamInterface<std::vector<int8_t>> {};
class ShortTest : public VectorTest, public ::testing::WithParamInterface<std::vector<int16_t>> {};
class IntTest : public VectorTest, public ::testing::WithParamInterface<std::vector<int32_t>> {};
class LongTest : public VectorTest, public ::testing::WithParamInterface<std::vector<int64_t>> {};

template<typename T>
int64_t calculateSum(T *data, int size) {
   int64_t result = 0;
   for (int i = 0; i<size; i++)
      result += data[i];

   return result;
}

template<typename T>
void tester(int laneSize, char sizeChar,  std::vector<T>input) {
   char inputTrees[1024];
   char *formatStr = "(method return= NoType args=[Address,Address] "
                      " (block "
                         " (%cstorei "
                             " (aload parm=0) "
                             " (vreductionAddVector128Int%d "
                                  " (vloadiVector128Int%d "
                                     " (aload parm=1)))) "
                         " (return))) ";

   sprintf(inputTrees, formatStr,
           sizeChar,
           laneSize,
           laneSize);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<void (*)(T*,T*)>();
  
    T expectedOutput = 0;
    for (T num: input){
      expectedOutput += num;
    }
    T output = 0;

    entry_point(&output,&input.front());

    EXPECT_EQ(expectedOutput, output);
}

TEST_P(ByteTest, integer) {
   std::vector<int8_t> input_vector = GetParam();
   tester(8, 'b', input_vector);
}
TEST_P(ShortTest, integer) {
   std::vector<int16_t> input_vector = GetParam();
   tester(16, 's', input_vector);
}
TEST_P(IntTest, integer) {
   std::vector<int32_t> input_vector = GetParam();
   tester(32, 'i', input_vector);
}
TEST_P(LongTest, integer) {
   std::vector<int64_t> input_vector = GetParam();
   tester(64, 'l', input_vector);
}



INSTANTIATE_TEST_CASE_P(b, ByteTest, testing::ValuesIn({
      std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      std::vector<int8_t>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      std::vector<int8_t>{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
      std::vector<int8_t>{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      std::vector<int8_t>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      std::vector<int8_t>{0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0},
      std::vector<int8_t>{0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15},
      std::vector<int8_t>{127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127},
      std::vector<int8_t>{-128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128},
      std::vector<int8_t>{127, -128, 127, -128, 127, -128, 127, -128, 127, -128, 127, -128, 127, -128, 127, -128}
   }));

INSTANTIATE_TEST_CASE_P(s, ShortTest, testing::ValuesIn({
      std::vector<int16_t>{0, 1, 2, 3, 4, 5, 6, 7},
      std::vector<int16_t>{0, 0, 0, 0, 0, 0, 0, 0},
      std::vector<int16_t>{-1, -1, -1, -1, -1, -1, -1, -1},
      std::vector<int16_t>{0, 0, 0, -1, 0, 0, 0, 0},
      std::vector<int16_t>{0, -1, 2, -3, 4, -5, 6, -7},
      std::vector<int16_t>{32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767},
      std::vector<int16_t>{-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768},
      std::vector<int16_t>{32767, -32768, 32767, -32768, 32767, -32768, 32767, -32768}
   }));

INSTANTIATE_TEST_CASE_P(i, IntTest, testing::ValuesIn({
      std::vector<int32_t>{0, 1, 2, 3},
      std::vector<int32_t>{0, 0, 0, 0},
      std::vector<int32_t>{-1, -1, -1, -1},
      std::vector<int32_t>{0, -1, 2, -3},
      std::vector<int32_t>{2147483647, 2147483647, 2147483647, 2147483647},
      std::vector<int32_t>{-2147483648, -2147483648, -2147483648, -2147483648},
      std::vector<int32_t>{2147483647, -2147483648, 2147483647, -2147483648}
   }));

INSTANTIATE_TEST_CASE_P(l, LongTest, testing::ValuesIn({
      std::vector<int64_t>{0, 1},
      std::vector<int64_t>{0, 0},
      std::vector<int64_t>{-1, -1},
      std::vector<int64_t>{0, -1},
      std::vector<int64_t>{9223372036854775807, 9223372036854775807},
      std::vector<int64_t>{-9223372036854775808, -9223372036854775808},
      std::vector<int64_t>{9223372036854775807, -9223372036854775808}
   }));
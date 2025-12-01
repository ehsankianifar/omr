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
#include <cmath>
class VectorCmpTest : public TRTest::JitTest {};

class ByteCmpTest : public VectorCmpTest, public ::testing::WithParamInterface<std::tuple<char*, std::vector<int8_t>, std::vector<int8_t>, std::vector<int8_t>, std::vector<int8_t>>> {};
// class ShortCmpTest : public VectorCmpTest, public ::testing::WithParamInterface<std::vector<int16_t>> {};
// class IntCmpTest : public VectorCmpTest, public ::testing::WithParamInterface<std::vector<int32_t>> {};
// class LongCmpTest : public VectorCmpTest, public ::testing::WithParamInterface<std::vector<int64_t>> {};
// class DoubleCmpTest : public VectorCmpTest, public ::testing::WithParamInterface<std::vector<double>> {};
// class FloatCmpTest : public VectorCmpTest, public ::testing::WithParamInterface<std::vector<float>> {};


template<typename T>
void tester(char *laneType, char *opCode,  std::vector<T>a,  std::vector<T>b,  std::vector<T>mask,  std::vector<T>expectedResult) {
   char inputTrees[1024];
   char *formatStr = "(method return= NoType args=[Address,Address,Address] "
                      " (block "
                         " (mstoreiVector128%s offset=0 "
                             " (aload parm=0) "
                             " (%sVector128%s "
                                  " (vloadiVector128%s (aload parm=1)) "
                                  " (vloadiVector128%s (aload parm=2)) "
                                  " (mloadiVector128%s (aload parm=3)))) "
                         " (return))) ";

   sprintf(inputTrees, formatStr,
           laneType,
           opCode,
           laneType,
           laneType,
           laneType,
           laneType);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<void (*)(T*,T*,T*,T*,void*)>();
  
    T output = 0;

    entry_point(&output,&a.front(),&b.front(),&mask.front());

   //  if(sizeChar == 'f') {
   //    // workaround for precision issue with float!
   //    EXPECT_EQ(std::round(expectedOutput * 1000), std::round(output * 1000));
   //  } else {
   //    EXPECT_EQ(expectedOutput, output);
   //  }
   //EXPECT_EQ(expectedResult, output);
   //EXPECT_THAT(output, testing::ContainerEq(expectedResult));
   for (int i = 0 ; i < expectedResult.size(), i++) {
      EXPECT_EQ(expectedResult[i], output[i]);
   }
}

TEST_P(ByteCmpTest, integer) {
   char *opCode = std::get<0>(GetParam());
   std::vector<int8_t> a_vector = std::get<1>(GetParam());
   std::vector<int8_t> b_vector = std::get<2>(GetParam());
   std::vector<int8_t> mask_vector = std::get<3>(GetParam());
   std::vector<int8_t> expected_vector = std::get<4>(GetParam());
   tester("Int8", opCode, a_vector, b_vector, mask_vector, expected_vector);
}
// TEST_P(ShortCmpTest, integer) {
//    std::vector<int16_t> input_vector = GetParam();
//    tester("Int16", 's', input_vector);
// }
// TEST_P(IntCmpTest, integer) {
//    std::vector<int32_t> input_vector = GetParam();
//    tester("Int32", 'i', input_vector);
// }
// TEST_P(LongCmpTest, integer) {
//    std::vector<int64_t> input_vector = GetParam();
//    tester("Int64", 'l', input_vector);
// }
// TEST_P(DoubleCmpTest, floatPoint) {
//    std::vector<double> input_vector = GetParam();
//    tester("Double", 'd', input_vector);
// }
// TEST_P(FloatCmpTest, floatPoint) {
//    std::vector<float> input_vector = GetParam();
//    tester("Float", 'f', input_vector);
// }
// TEST_P(FloatCmpTest, floatPoint2ref) {
//    std::vector<float> input_vector = GetParam();
//    tester2("Float", 'f', input_vector);
// }

INSTANTIATE_TEST_CASE_P(b, ByteCmpTest, testing::ValuesIn({
      std::make_tuple("vcmpeq",
         std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, 
         std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
         std::vector<int8_t>{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
         std::vector<int8_t>{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1})
   }));

// INSTANTIATE_TEST_CASE_P(s, ShortCmpTest, testing::ValuesIn({
//       std::vector<int16_t>{0, 1, 2, 3, 4, 5, 6, 7},
//       std::vector<int16_t>{0, 0, 0, 0, 0, 0, 0, 0},
//       std::vector<int16_t>{-1, -1, -1, -1, -1, -1, -1, -1},
//       std::vector<int16_t>{0, 0, 0, -1, 0, 0, 0, 0},
//       std::vector<int16_t>{0, -1, 2, -3, 4, -5, 6, -7},
//       std::vector<int16_t>{32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767},
//       std::vector<int16_t>{-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768},
//       std::vector<int16_t>{32767, -32768, 32767, -32768, 32767, -32768, 32767, -32768}
//    }));

// INSTANTIATE_TEST_CASE_P(i, IntCmpTest, testing::ValuesIn({
//       std::vector<int32_t>{0, 1, 2, 3},
//       std::vector<int32_t>{0, 0, 0, 0},
//       std::vector<int32_t>{-1, -1, -1, -1},
//       std::vector<int32_t>{0, -1, 2, -3},
//       std::vector<int32_t>{2147483647, 2147483647, 2147483647, 2147483647},
//       std::vector<int32_t>{-2147483648, -2147483648, -2147483648, -2147483648},
//       std::vector<int32_t>{2147483647, -2147483648, 2147483647, -2147483648}
//    }));

// INSTANTIATE_TEST_CASE_P(l, LongCmpTest, testing::ValuesIn({
//       std::vector<int64_t>{0, 1},
//       std::vector<int64_t>{0, 0},
//       std::vector<int64_t>{-1, -1},
//       std::vector<int64_t>{0, -1},
//       std::vector<int64_t>{9223372036854775807, 9223372036854775807},
//       std::vector<int64_t>{-9223372036854775808, -9223372036854775808},
//       std::vector<int64_t>{9223372036854775807, -9223372036854775808}
//    }));

// INSTANTIATE_TEST_CASE_P(d, DoubleCmpTest, testing::ValuesIn({
//       std::vector<double>{0, 1},
//       std::vector<double>{0.1, 1.4},
//       std::vector<double>{1000.222, -222.434}
//    }));

// INSTANTIATE_TEST_CASE_P(f, FloatCmpTest, testing::ValuesIn({
//       std::vector<float>{0, 1, 2, 3},
//       std::vector<float>{0.1, 1.4, 22.4, 18.9},
//       std::vector<float>{99.2, -22.4, 34.8, -86.2}
//    }));
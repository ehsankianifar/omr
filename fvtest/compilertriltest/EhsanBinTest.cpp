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
class VectorBinTest : public TRTest::JitTest {};

class ByteBinTest : public VectorBinTest,   public ::testing::WithParamInterface<std::tuple<const char*, std::vector<int8_t>,  std::vector<int8_t>,  std::vector<int8_t>,  std::vector<int8_t>>> {};
class ShortBinTest : public VectorBinTest,  public ::testing::WithParamInterface<std::tuple<const char*, std::vector<int16_t>, std::vector<int16_t>, std::vector<int16_t>, std::vector<int16_t>>> {};
class IntBinTest : public VectorBinTest,    public ::testing::WithParamInterface<std::tuple<const char*, std::vector<int32_t>, std::vector<int32_t>, std::vector<int32_t>, std::vector<int32_t>>> {};
class LongBinTest : public VectorBinTest,   public ::testing::WithParamInterface<std::tuple<const char*, std::vector<int64_t>, std::vector<int64_t>, std::vector<int64_t>, std::vector<int64_t>>> {};


template<typename T1, typename T2>
void tester3(const char *laneType, const char *opCode,  std::vector<T1>a,  std::vector<T1>b,  std::vector<T2>mask,  std::vector<T2>expectedResult) {
   char inputTrees[1024];
   char *formatStr = "(method return= NoType args=[Address,Address,Address,Address] "
                      " (block "
                         " (vstoreiVector128%s offset=0 "
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

    auto entry_point = compiler.getEntryPoint<void (*)(T2*,T1*,T1*,T2*)>();
  
    std::vector<T2> output(128/sizeof(T2));
    entry_point(&output.front(),&a.front(),&b.front(),&mask.front());

   for (int i = 0 ; i < expectedResult.size(); i++) {
      EXPECT_EQ(expectedResult[i], output[i]);
   }
}

template<typename T1, typename T2>
void tester4(const char *laneType, const char *opCode,  std::vector<T1>a,  std::vector<T1>b,  std::vector<T2>expectedResult) {
   char inputTrees[1024];
   char *formatStr = "(method return= NoType args=[Address,Address,Address] "
                      " (block "
                         " (vstoreiVector128%s offset=0 "
                             " (aload parm=0) "
                             " (%sVector128%s "
                                  " (vloadiVector128%s (aload parm=1)) "
                                  " (vloadiVector128%s (aload parm=2)))) "
                         " (return))) ";

   sprintf(inputTrees, formatStr,
           laneType,
           opCode,
           laneType,
           laneType,
           laneType);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<void (*)(T2*,T1*,T1*)>();
  
    std::vector<T2> output(128/sizeof(T2));
    entry_point(&output.front(),&a.front(),&b.front());

   for (int i = 0 ; i < expectedResult.size(); i++) {
      EXPECT_EQ(expectedResult[i], output[i]);
   }
}


TEST_P(ByteBinTest, integer) {
   const char *opCode = std::get<0>(GetParam());
   std::vector<int8_t> a_vector = std::get<1>(GetParam());
   std::vector<int8_t> b_vector = std::get<2>(GetParam());
   std::vector<int8_t> mask_vector = std::get<3>(GetParam());
   std::vector<int8_t> expected_vector = std::get<4>(GetParam());
   if(opCode[1]=='m'){
      tester3("Int8", opCode, a_vector, b_vector, mask_vector, expected_vector);
   } else {
      tester4("Int8", opCode, a_vector, b_vector, expected_vector);
   }

}
TEST_P(ShortBinTest, integer) {
   const char *opCode = std::get<0>(GetParam());
   std::vector<int16_t> a_vector = std::get<1>(GetParam());
   std::vector<int16_t> b_vector = std::get<2>(GetParam());
   std::vector<int16_t> mask_vector = std::get<3>(GetParam());
   std::vector<int16_t> expected_vector = std::get<4>(GetParam());
   if(opCode[1]=='m'){
      tester3("Int16", opCode, a_vector, b_vector, mask_vector, expected_vector);
   } else {
      tester4("Int16", opCode, a_vector, b_vector, expected_vector);
   }
}
TEST_P(IntBinTest, integer) {
   const char *opCode = std::get<0>(GetParam());
   std::vector<int32_t> a_vector = std::get<1>(GetParam());
   std::vector<int32_t> b_vector = std::get<2>(GetParam());
   std::vector<int32_t> mask_vector = std::get<3>(GetParam());
   std::vector<int32_t> expected_vector = std::get<4>(GetParam());
   if(opCode[1]=='m'){
      tester3("Int32", opCode, a_vector, b_vector, mask_vector, expected_vector);
   } else {
      tester4("Int32", opCode, a_vector, b_vector, expected_vector);
   }
}
TEST_P(LongBinTest, integer) {
   const char *opCode = std::get<0>(GetParam());
   std::vector<int64_t> a_vector = std::get<1>(GetParam());
   std::vector<int64_t> b_vector = std::get<2>(GetParam());
   std::vector<int64_t> mask_vector = std::get<3>(GetParam());
   std::vector<int64_t> expected_vector = std::get<4>(GetParam());
   if(opCode[1]=='m'){
      tester3("Int64", opCode, a_vector, b_vector, mask_vector, expected_vector);
   } else {
      tester4("Int64", opCode, a_vector, b_vector, expected_vector);
   }
}

INSTANTIATE_TEST_CASE_P(vcompressbits, ByteBinTest, testing::ValuesIn({
      std::make_tuple("vcompressbits",
         std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 4, -15}, 
         std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -15, 4},
         std::vector<int8_t>{0},
         std::vector<int8_t>{0, 1, 1, 3, 1, 3, 3, 7, 1, 3,  3, 7, 3, 7, 0, 0})
   }));

INSTANTIATE_TEST_CASE_P(vexpandbits, ByteBinTest, testing::ValuesIn({
      std::make_tuple("vexpandbits",
         std::vector<int8_t>{0, 1, 1, 3, 1, 3, 3, 7, 1, 3,  3, 7,  3,  7,   0,   -5},
         std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -15,  2},
         std::vector<int8_t>{0},
         std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 2})
   }));
INSTANTIATE_TEST_CASE_P(vexpandbits, IntBinTest, testing::ValuesIn({
      std::make_tuple("vexpandbits",
         std::vector<int8_t>{0, 1, 1, 3, 1, 3, 3, -5},
         std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 2},
         std::vector<int8_t>{0},
         std::vector<int8_t>{0, 1, 2, 3, 4, 5, 6, 2})
   }));

INSTANTIATE_TEST_CASE_P(vmexpandbits, ByteBinTest, testing::ValuesIn({
      std::make_tuple("vmexpandbits",
         std::vector<int8_t>{0,  1, 1,  3,  1, 3, 3,  7, 1,  3,  3,  7,   3,  7,   0,  0},
         std::vector<int8_t>{0,  1, 2,  3,  4, 5, 6,  7, 8,  9, 10, 11,  12, 13, -15,  4},
         std::vector<int8_t>{-1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1,  0,  -1,  0,  -1,  0},
         std::vector<int8_t>{0,  1, 2,  3,  4, 3, 6,  7, 8,  3, 10,  7,  12,  7,   0,  0})
   }));

// INSTANTIATE_TEST_CASE_P(s, ShortBinTest, testing::ValuesIn({
//       std::vector<int16_t>{0, 1, 2, 3, 4, 5, 6, 7},
//       std::vector<int16_t>{0, 0, 0, 0, 0, 0, 0, 0},
//       std::vector<int16_t>{-1, -1, -1, -1, -1, -1, -1, -1},
//       std::vector<int16_t>{0, 0, 0, -1, 0, 0, 0, 0},
//       std::vector<int16_t>{0, -1, 2, -3, 4, -5, 6, -7},
//       std::vector<int16_t>{32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767},
//       std::vector<int16_t>{-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768},
//       std::vector<int16_t>{32767, -32768, 32767, -32768, 32767, -32768, 32767, -32768}
//    }));

// INSTANTIATE_TEST_CASE_P(i, IntBinTest, testing::ValuesIn({
//       std::vector<int32_t>{0, 1, 2, 3},
//       std::vector<int32_t>{0, 0, 0, 0},
//       std::vector<int32_t>{-1, -1, -1, -1},
//       std::vector<int32_t>{0, -1, 2, -3},
//       std::vector<int32_t>{2147483647, 2147483647, 2147483647, 2147483647},
//       std::vector<int32_t>{-2147483648, -2147483648, -2147483648, -2147483648},
//       std::vector<int32_t>{2147483647, -2147483648, 2147483647, -2147483648}
//    }));

// INSTANTIATE_TEST_CASE_P(l, LongBinTest, testing::ValuesIn({
//       std::vector<int64_t>{0, 1},
//       std::vector<int64_t>{0, 0},
//       std::vector<int64_t>{-1, -1},
//       std::vector<int64_t>{0, -1},
//       std::vector<int64_t>{9223372036854775807, 9223372036854775807},
//       std::vector<int64_t>{-9223372036854775808, -9223372036854775808},
//       std::vector<int64_t>{9223372036854775807, -9223372036854775808}
//    }));

// INSTANTIATE_TEST_CASE_P(d, DoubleBinTest, testing::ValuesIn({
//       std::vector<double>{0, 1},
//       std::vector<double>{0.1, 1.4},
//       std::vector<double>{1000.222, -222.434}
//    }));

// INSTANTIATE_TEST_CASE_P(f, FloatBinTest, testing::ValuesIn({
//       std::vector<float>{0, 1, 2, 3},
//       std::vector<float>{0.1, 1.4, 22.4, 18.9},
//       std::vector<float>{99.2, -22.4, 34.8, -86.2}
//    }));
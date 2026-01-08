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
class VectorUniTest : public TRTest::JitTest {};

class ByteUniTest : public VectorUniTest,   public ::testing::WithParamInterface<std::tuple<const char*, std::vector<int8_t>,  std::vector<int8_t>,  std::vector<int8_t>,  std::vector<int8_t>>> {};
class ShortUniTest : public VectorUniTest,  public ::testing::WithParamInterface<std::tuple<const char*, std::vector<int16_t>, std::vector<int16_t>, std::vector<int16_t>, std::vector<int16_t>>> {};
class IntUniTest : public VectorUniTest,    public ::testing::WithParamInterface<std::tuple<const char*, std::vector<int32_t>, std::vector<int32_t>, std::vector<int32_t>, std::vector<int32_t>>> {};
class LongUniTest : public VectorUniTest,   public ::testing::WithParamInterface<std::tuple<const char*, std::vector<int64_t>, std::vector<int64_t>, std::vector<int64_t>, std::vector<int64_t>>> {};


template<typename T>
void tester5(const char *laneType, const char *opCode,  std::vector<T>a,  std::vector<T>expectedResult) {
   char inputTrees[1024];
   char *formatStr = "(method return= NoType args=[Address,Address,Address,Address] "
                      " (block "
                         " (mstoreiVector128%s offset=0 "
                             " (aload parm=0) "
                             " (%sMask128%s "
                                  " (mloadiVector128%s (aload parm=1)))) "
                         " (return))) ";

   sprintf(inputTrees, formatStr,
           laneType,
           opCode,
           laneType,
           laneType);
   auto trees = parseString(inputTrees);

   ASSERT_NOTNULL(trees);

   Tril::DefaultCompiler compiler(trees);
   ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

   auto entry_point = compiler.getEntryPoint<void (*)(T*,T*)>();
  
   std::vector<T> output(128/sizeof(T));
   entry_point(&output.front(),&a.front());

   for (int i = 0 ; i < expectedResult.size(); i++) {
      EXPECT_EQ(expectedResult[i], output[i]);
   }
}



TEST_P(ByteUniTest, integer) {
   const char *opCode = std::get<0>(GetParam());
   std::vector<int8_t> a_vector = std::get<1>(GetParam());
   std::vector<int8_t> expected_vector = std::get<4>(GetParam());
   tester5("Int8", opCode, a_vector, expected_vector);
}


INSTANTIATE_TEST_CASE_P(mcompress, ByteUniTest, testing::ValuesIn({
      std::make_tuple("mcompress",
         std::vector<int8_t>{0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1}, 
         std::vector<int8_t>{0},
         std::vector<int8_t>{0},
         std::vector<int8_t>{-1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
      std::make_tuple("mcompress",
         std::vector<int8_t>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
         std::vector<int8_t>{0},
         std::vector<int8_t>{0},
         std::vector<int8_t>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
      std::make_tuple("mcompress",
         std::vector<int8_t>{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
         std::vector<int8_t>{0},
         std::vector<int8_t>{0},
         std::vector<int8_t>{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1})
   }));
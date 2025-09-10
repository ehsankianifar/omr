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

#define MAX_NUM_LANES 64

class VectorTest : public TRTest::JitTest {};

class ParameterizedUnaryMaskTest : public VectorTest, public ::testing::WithParamInterface<std::tuple<const char *, TR::DataTypes, const char *, const char *>> {};


static char* getTypeString(TR::DataTypes type){
   switch(type){
      case TR::Int8 :
         return "Vector128Int8";
      case TR::Int16 :
         return "Vector128Int16";
      case TR::Int32 :
         return "Vector128Int32";
      case TR::Int64 :
         return "Vector128Int64";
      case TR::Double :
         return "Vector128Double";
      case TR::Float :
         return "Vector128Float";
      default :
         return "WRONG TYPE";
   }
}

uint8_t *convertResultCharToArray(const char *input) {
   uint8_t result[16];
   for (int i=0; i<16; i++) {
      if (input[i]=='0')
         result[i] = 0;
      else
         result[i] = 0xff;
   }
   return result;
}

uint8_t *convertInputCharToArray(const char *input) {
   uint8_t result[8];
   for (int i=0; i<8; i++) {
      if (input[i]=='0')
         result[i] = 0;
      else
         result[i] = 1;
   }
   return result;
}

TEST_P(ParameterizedUnaryMaskTest, loadIndirect) {
   const char *size = std::get<0>(GetParam());
   TR::DataTypes type = std::get<1>(GetParam());
   const char *inputChar = std::get<2>(GetParam());
   const char *resultChar = std::get<3>(GetParam());
   const char *typeString = getTypeString(type);

   char inputTrees[1024];
   char *formatStr = "(method return= NoType args=[Address,Address]                   "
                     "  (block                                                        "
                     "     (vstoreiVector128Int8 offset=0                             "
                     "         (aload parm=0)                                         "
                     "         (%s2m%s                                                "
                     "              (%sloadi                                          "
                     "                 (aload parm=1))))                              "
                     "     (return)))                                                 ";

   sprintf(inputTrees, formatStr,
           size,
           typeString,
           size);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<void (*)(int8_t[],int8_t[])>();
    // This test currently assumes 128bit SIMD

    int8_t output[] =  {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
    int8_t *inputA =  convertInputCharToArray(inputChar);
    int8_t *expectedOutput = convertResultCharToArray(resultChar);

    entry_point(output,inputA);

    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(expectedOutput[i], output[i]);
    }
}

INSTANTIATE_TEST_CASE_P(qq, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *>(
   std::make_tuple("b", TR::Int8 , "00000000", "0000000000000000"),
   std::make_tuple("b", TR::Int8 , "10000000", "1111111111111111")
)));

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

class ParameterizedUnaryMaskTest : public VectorTest, public ::testing::WithParamInterface<std::tuple<const char *, const char *, const char *, const char *>> {};


uint8_t *convertResultCharToArray(const char *input) {
   uint8_t *result =(uint8_t*)malloc(16);
   memset((void*)result, 0, 16);
   int index = 0;
   for (int i=0; input[i]!='\0'; i++) {
      if (input[i]=='0') {
         result[index] = 0;
         index++;
      }
      else if (input[i]=='1') {
         result[index] = 0xff;
         index++;
      }
   }
   return result;
}

uint8_t *convertInputCharToArray(const char *input) {
   uint8_t *result =(uint8_t*)malloc(8);
   memset((void*)result, 0, 8);
   int index = 0;
   for (int i=0; input[i]!='\0'; i++) {
      if (input[i]=='0') {
         result[index] = 0;
         index++;
      }
      else if (input[i]=='1') {
         result[index] = 1;
         index++;
      }
   }
   return result;
}

TEST_P(ParameterizedUnaryMaskTest, loadIndirect) {
   const char *size = std::get<0>(GetParam());
   const char *typeString = std::get<1>(GetParam());
   const char *inputChar = std::get<2>(GetParam());
   const char *resultChar = std::get<3>(GetParam());

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

    auto entry_point = compiler.getEntryPoint<void (*)(uint8_t*,uint8_t*)>();
    // This test currently assumes 128bit SIMD

    uint8_t output[] =  {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
    uint8_t *inputA =  convertInputCharToArray(inputChar);
    uint8_t *expectedOutput = convertResultCharToArray(resultChar);

    entry_point(output,inputA);

    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(expectedOutput[i], output[i]);
    }
    free(inputA);
    free(expectedOutput);
}

TEST_P(ParameterizedUnaryMaskTest, loadConst) {
   const char *size = std::get<0>(GetParam());
   const char *typeString = std::get<1>(GetParam());
   const char *inputChar = std::get<2>(GetParam());
   const char *resultChar = std::get<3>(GetParam());
   uint8_t *input =  convertInputCharToArray(inputChar);
   char *value = "0x                ";
   int bytesToSet = 0;
   switch (size[0]) {
      case 'b':
         bytesToSet = 1;
         break;
      case 's':
         bytesToSet = 2;
         break;
      case 'i':
         bytesToSet = 4;
         break;
      case 'l':
         bytesToSet = 8;
         break;
   }
   int valueIndex=2;
   for (int i=0; i<bytesToSet; i++)
   {
      value[valueIndex] = '0';
      valueIndex++;
      value[valueIndex] = (char)((unit8_t)'0'+input[i]);
      valueIndex++;
   }

   char inputTrees[1024];
   char *formatStr = "(method return= NoType args=[Address]                   "
                     "  (block                                                        "
                     "     (vstoreiVector128Int8 offset=0                             "
                     "         (aload parm=0)                                         "
                     "         (%s2m%s                                                "
                     "              (%sconst %s)))                        "
                     "     (return)))                                                 ";

   sprintf(inputTrees, formatStr,
           size,
           typeString,
           size,
           value);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<void (*)(uint8_t*)>();
    // This test currently assumes 128bit SIMD

    uint8_t output[] =  {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
    
    uint8_t *expectedOutput = convertResultCharToArray(resultChar);

    entry_point(output);

    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(expectedOutput[i], output[i]);
    }
    free(input);
    free(expectedOutput);
}

INSTANTIATE_TEST_CASE_P(indirect, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, const char *, const char *, const char *>>(
   std::make_tuple("b", "Vector128Int64" , "0", "00000000_00000000"),
   std::make_tuple("b", "Vector128Int64" , "1", "11111111_11111111"),
   std::make_tuple("s", "Vector128Int64" , "11", "11111111_11111111"),
   std::make_tuple("s", "Vector128Int64" , "01", "00000000_11111111"),
   std::make_tuple("s", "Vector128Int64" , "10", "11111111_00000000"),
   std::make_tuple("s", "Vector128Int64" , "11", "11111111_11111111"),
   std::make_tuple("i", "Vector128Int32" , "0111", "0000_1111_1111_1111"),
   std::make_tuple("i", "Vector128Int32" , "1011", "1111_0000_1111_1111"),
   std::make_tuple("i", "Vector128Int32" , "1101", "1111_1111_0000_1111"),
   std::make_tuple("i", "Vector128Int32" , "1110", "1111_1111_1111_0000"),
   std::make_tuple("i", "Vector128Int32" , "0000", "0000_0000_0000_0000"),
   std::make_tuple("i", "Vector128Int32" , "1111", "1111_1111_1111_1111"),
   std::make_tuple("l", "Vector128Int16" , "0000_0000", "00_00_00_00_00_00_00_00"),
   std::make_tuple("l", "Vector128Int16" , "1111_1111", "11_11_11_11_11_11_11_11"),
   std::make_tuple("l", "Vector128Int16" , "0111_1111", "00_11_11_11_11_11_11_11"),
   std::make_tuple("l", "Vector128Int16" , "1111_1110", "11_11_11_11_11_11_11_00"),
   std::make_tuple("l", "Vector128Int16" , "1010_1010", "11_00_11_00_11_00_11_00")
)));

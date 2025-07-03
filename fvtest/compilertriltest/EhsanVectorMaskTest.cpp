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


class ParameterizedUnaryMaskTest : public VectorTest, public ::testing::WithParamInterface<std::tuple<const char *, TR::DataTypes, const char *, int>> {};
class ParameterizedUnaryMaskTest2 : public VectorTest, public ::testing::WithParamInterface<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>> {};


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

const int DEFINITION_OF_TRUE_IN_MASK = -1;

static void buildVectorArray(TR::DataTypes type, const char* values, void* vectorArray){
   switch(type){
      case TR::Int8 :
         for(int i = 0; i<16; i++)
         {
            reinterpret_cast<uint8_t*>(vectorArray)[i]= (values[i]=='1') ? ((int8_t)DEFINITION_OF_TRUE_IN_MASK) : 0;
         }
      break;
      case TR::Int16 :
         for(int i = 0; i<8; i++)
         {
            reinterpret_cast<uint16_t*>(vectorArray)[i]= (values[i]=='1') ? ((int16_t)DEFINITION_OF_TRUE_IN_MASK) : 0;
         }
      break;
      case TR::Int32 :
         for(int i = 0; i<4; i++)
         {
            reinterpret_cast<uint32_t*>(vectorArray)[i]= (values[i]=='1') ? ((int32_t)DEFINITION_OF_TRUE_IN_MASK) : 0;
         }
      break;
      case TR::Int64 :
      case TR::Double :
      case TR::Float :
         for(int i = 0; i<2; i++)
         {
            reinterpret_cast<uint64_t*>(vectorArray)[i]= (values[i]=='1') ? ((int64_t)DEFINITION_OF_TRUE_IN_MASK) : 0;
         }
      break;
   }
}


TEST_P(ParameterizedUnaryMaskTest, m) {
   const char *opCode = std::get<0>(GetParam());
   TR::DataTypes type = std::get<1>(GetParam());
   const char *arrayChar = std::get<2>(GetParam());
   int result = std::get<3>(GetParam());
   const char *typeString = getTypeString(type);
   void * vectorArray = malloc(16);
   buildVectorArray(type, arrayChar, vectorArray);

   char inputTrees[1024];

   char *formatStr = "(method return= NoType args=[Address] \n"
                     "  (block \n"
                     "    (ireturn \n"
                     "      (%s%s offset=0 \n"
                     "         (vloadi%s (aload parm=0)))))) \n";

   sprintf(inputTrees, formatStr,
           opCode,
           typeString,
           typeString);

   auto trees = parseString(inputTrees);

   Tril::DefaultCompiler compiler(trees);

   ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

   auto entry_point = compiler.getEntryPoint<int32_t (*) (void*)>();

   int32_t out = entry_point(vectorArray);
   EXPECT_EQ(out, result);
   free(vectorArray);
}

TEST_P(ParameterizedUnaryMaskTest2, mm) {
   const char *opCode = std::get<0>(GetParam());
   TR::DataTypes type = std::get<1>(GetParam());
   const char *arrayChar1 = std::get<2>(GetParam());
   const char *arrayChar2 = std::get<3>(GetParam());
   int result = std::get<4>(GetParam());
   const char *typeString = getTypeString(type);
   void * vectorArray1 = malloc(16);
   buildVectorArray(type, arrayChar1, vectorArray1);
   void * vectorArray2 = malloc(16);
   buildVectorArray(type, arrayChar2, vectorArray2);

   char inputTrees[1024];

   char *formatStr = "(method return= NoType args=[Address,Address] \n"
                     "  (block \n"
                     "    (ireturn \n"
                     "      (%s%s offset=0 \n"
                     "        (vloadi%s (aload parm=0)) \n"
                     "        (vloadi%s (aload parm=1)))))) \n";

   sprintf(inputTrees, formatStr,
           opCode,
           typeString,
           typeString,
           typeString);

   auto trees = parseString(inputTrees);

   Tril::DefaultCompiler compiler(trees);

   ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

   auto entry_point = compiler.getEntryPoint<int32_t (*) (void*,void*)>();

   int32_t out = entry_point(vectorArray1,vectorArray2);
   EXPECT_EQ(out, result);
   free(vectorArray1);
   free(vectorArray2);
}

INSTANTIATE_TEST_CASE_P(mAll8, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAllTrue", TR::Int8  , "0000000000000000", 0),
   std::make_tuple("mAllTrue", TR::Int8  , "0101010101010101", 0),
   std::make_tuple("mAllTrue", TR::Int8  , "0000000000000001", 0),
   std::make_tuple("mAllTrue", TR::Int8  , "1111111111111111", 1)
)));

INSTANTIATE_TEST_CASE_P(mAll16, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAllTrue", TR::Int16 , "00000000", 0),
   std::make_tuple("mAllTrue", TR::Int16 , "01010101", 0),
   std::make_tuple("mAllTrue", TR::Int16 , "00000001", 0),
   std::make_tuple("mAllTrue", TR::Int16 , "11111111", 1)
)));

INSTANTIATE_TEST_CASE_P(mAll32, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAllTrue", TR::Int32 , "0000", 0),
   std::make_tuple("mAllTrue", TR::Int32 , "0101", 0),
   std::make_tuple("mAllTrue", TR::Int32 , "0001", 0),
   std::make_tuple("mAllTrue", TR::Int32 , "1111", 1)
)));

INSTANTIATE_TEST_CASE_P(mAll64, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAllTrue", TR::Int64 , "00", 0),
   std::make_tuple("mAllTrue", TR::Int64 , "01", 0),
   std::make_tuple("mAllTrue", TR::Int64 , "10", 0),
   std::make_tuple("mAllTrue", TR::Int64 , "11", 1)
)));

INSTANTIATE_TEST_CASE_P(mAllDouble, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAllTrue", TR::Double , "00", 0),
   std::make_tuple("mAllTrue", TR::Double , "01", 0),
   std::make_tuple("mAllTrue", TR::Double , "10", 0),
   std::make_tuple("mAllTrue", TR::Double , "11", 1)
)));

INSTANTIATE_TEST_CASE_P(mAllFloat, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAllTrue", TR::Float , "00", 0),
   std::make_tuple("mAllTrue", TR::Float , "01", 0),
   std::make_tuple("mAllTrue", TR::Float , "10", 0),
   std::make_tuple("mAllTrue", TR::Float , "11", 1)
)));


INSTANTIATE_TEST_CASE_P(mAny8, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAnyTrue", TR::Int8  , "0000000000000000", 0),
   std::make_tuple("mAnyTrue", TR::Int8  , "0101010101010101", 1),
   std::make_tuple("mAnyTrue", TR::Int8  , "0000000000000001", 1),
   std::make_tuple("mAnyTrue", TR::Int8  , "1111111111111111", 1)
)));

INSTANTIATE_TEST_CASE_P(mAny16, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAnyTrue", TR::Int16 , "00000000", 0),
   std::make_tuple("mAnyTrue", TR::Int16 , "01010101", 1),
   std::make_tuple("mAnyTrue", TR::Int16 , "00000001", 1),
   std::make_tuple("mAnyTrue", TR::Int16 , "11111111", 1)
)));

INSTANTIATE_TEST_CASE_P(mAny32, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAnyTrue", TR::Int32 , "0000", 0),
   std::make_tuple("mAnyTrue", TR::Int32 , "0101", 1),
   std::make_tuple("mAnyTrue", TR::Int32 , "0001", 1),
   std::make_tuple("mAnyTrue", TR::Int32 , "1111", 1)
)));

INSTANTIATE_TEST_CASE_P(mAny64, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAnyTrue", TR::Int64 , "00", 0),
   std::make_tuple("mAnyTrue", TR::Int64 , "01", 1),
   std::make_tuple("mAnyTrue", TR::Int64 , "10", 1),
   std::make_tuple("mAnyTrue", TR::Int64 , "11", 1)
)));

INSTANTIATE_TEST_CASE_P(mAnyDouble, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAnyTrue", TR::Double , "00", 0),
   std::make_tuple("mAnyTrue", TR::Double , "01", 1),
   std::make_tuple("mAnyTrue", TR::Double , "10", 1),
   std::make_tuple("mAnyTrue", TR::Double , "11", 1)
)));

INSTANTIATE_TEST_CASE_P(mAnyFloat, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, int>>(
   std::make_tuple("mAnyTrue", TR::Float , "00", 0),
   std::make_tuple("mAnyTrue", TR::Float , "01", 1),
   std::make_tuple("mAnyTrue", TR::Float , "10", 1),
   std::make_tuple("mAnyTrue", TR::Float , "11", 1)
)));


//mmAllTrue and mmAnyTrue

INSTANTIATE_TEST_CASE_P(mmAll8, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAllTrue", TR::Int8  , "0000000000000000", "0000000000000000", 0),
   std::make_tuple("mmAllTrue", TR::Int8  , "0000000000000000", "1111111111111111", 0),
   std::make_tuple("mmAllTrue", TR::Int8  , "1111111111111111", "0000000000000000", 0),
   std::make_tuple("mmAllTrue", TR::Int8  , "1111111111111111", "1111111111111110", 0),
   std::make_tuple("mmAllTrue", TR::Int8  , "0111111111111111", "1111111111111111", 0),
   std::make_tuple("mmAllTrue", TR::Int8  , "1111111111111111", "1111111111111111", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAll16, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAllTrue", TR::Int16  , "00000000", "00000000", 0),
   std::make_tuple("mmAllTrue", TR::Int16  , "00000000", "11111111", 0),
   std::make_tuple("mmAllTrue", TR::Int16  , "11111111", "00000000", 0),
   std::make_tuple("mmAllTrue", TR::Int16  , "11111111", "11111110", 0),
   std::make_tuple("mmAllTrue", TR::Int16  , "01111111", "11111111", 0),
   std::make_tuple("mmAllTrue", TR::Int16  , "11111111", "11111111", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAll32, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAllTrue", TR::Int32  , "0000", "0000", 0),
   std::make_tuple("mmAllTrue", TR::Int32  , "0000", "1111", 0),
   std::make_tuple("mmAllTrue", TR::Int32  , "1111", "0000", 0),
   std::make_tuple("mmAllTrue", TR::Int32  , "1111", "1110", 0),
   std::make_tuple("mmAllTrue", TR::Int32  , "0111", "1111", 0),
   std::make_tuple("mmAllTrue", TR::Int32  , "1111", "1111", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAll64, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAllTrue", TR::Int64  , "00", "00", 0),
   std::make_tuple("mmAllTrue", TR::Int64  , "00", "11", 0),
   std::make_tuple("mmAllTrue", TR::Int64  , "11", "00", 0),
   std::make_tuple("mmAllTrue", TR::Int64  , "11", "10", 0),
   std::make_tuple("mmAllTrue", TR::Int64  , "01", "11", 0),
   std::make_tuple("mmAllTrue", TR::Int64  , "11", "11", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAllDouble, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAllTrue", TR::Double  , "00", "00", 0),
   std::make_tuple("mmAllTrue", TR::Double  , "00", "11", 0),
   std::make_tuple("mmAllTrue", TR::Double  , "11", "00", 0),
   std::make_tuple("mmAllTrue", TR::Double  , "11", "10", 0),
   std::make_tuple("mmAllTrue", TR::Double  , "01", "11", 0),
   std::make_tuple("mmAllTrue", TR::Double  , "11", "11", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAllFloat, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAllTrue", TR::Float  , "00", "00", 0),
   std::make_tuple("mmAllTrue", TR::Float  , "00", "11", 0),
   std::make_tuple("mmAllTrue", TR::Float  , "11", "00", 0),
   std::make_tuple("mmAllTrue", TR::Float  , "11", "10", 0),
   std::make_tuple("mmAllTrue", TR::Float  , "01", "11", 0),
   std::make_tuple("mmAllTrue", TR::Float  , "11", "11", 1)
)));

//mmAny
INSTANTIATE_TEST_CASE_P(mmAny8, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAnyTrue", TR::Int8  , "0000000000000000", "0000000000000000", 0),
   std::make_tuple("mmAnyTrue", TR::Int8  , "0000000000000000", "1111111111111111", 0),
   std::make_tuple("mmAnyTrue", TR::Int8  , "1111111111111111", "0000000000000000", 0),
   std::make_tuple("mmAnyTrue", TR::Int8  , "1111111100000000", "0000000011111111", 0),
   std::make_tuple("mmAnyTrue", TR::Int8  , "0000000000000001", "0000000000000001", 1),
   std::make_tuple("mmAnyTrue", TR::Int8  , "1000000000000000", "1000000000000000", 1),
   std::make_tuple("mmAnyTrue", TR::Int8  , "1111111111111111", "1111111111111111", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAny16, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAnyTrue", TR::Int16  , "00000000", "00000000", 0),
   std::make_tuple("mmAnyTrue", TR::Int16  , "00000000", "11111111", 0),
   std::make_tuple("mmAnyTrue", TR::Int16  , "11111111", "00000000", 0),
   std::make_tuple("mmAnyTrue", TR::Int16  , "11110000", "00001111", 0),
   std::make_tuple("mmAnyTrue", TR::Int16  , "00000001", "00000001", 1),
   std::make_tuple("mmAnyTrue", TR::Int16  , "10000000", "10000000", 1),
   std::make_tuple("mmAnyTrue", TR::Int16  , "11111111", "11111111", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAny32, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAnyTrue", TR::Int32  , "0000", "0000", 0),
   std::make_tuple("mmAnyTrue", TR::Int32  , "0000", "1111", 0),
   std::make_tuple("mmAnyTrue", TR::Int32  , "1111", "0000", 0),
   std::make_tuple("mmAnyTrue", TR::Int32  , "1100", "0011", 0),
   std::make_tuple("mmAnyTrue", TR::Int32  , "1000", "1000", 1),
   std::make_tuple("mmAnyTrue", TR::Int32  , "0001", "0001", 1),
   std::make_tuple("mmAnyTrue", TR::Int32  , "1111", "1111", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAny64, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAnyTrue", TR::Int64  , "00", "00", 0),
   std::make_tuple("mmAnyTrue", TR::Int64  , "00", "11", 0),
   std::make_tuple("mmAnyTrue", TR::Int64  , "11", "00", 0),
   std::make_tuple("mmAnyTrue", TR::Int64  , "10", "01", 0),
   std::make_tuple("mmAnyTrue", TR::Int64  , "10", "10", 1),
   std::make_tuple("mmAnyTrue", TR::Int64  , "01", "01", 1),
   std::make_tuple("mmAnyTrue", TR::Int64  , "11", "11", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAnyDouble, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAnyTrue", TR::Double  , "00", "00", 0),
   std::make_tuple("mmAnyTrue", TR::Double  , "00", "11", 0),
   std::make_tuple("mmAnyTrue", TR::Double  , "11", "00", 0),
   std::make_tuple("mmAnyTrue", TR::Double  , "10", "01", 0),
   std::make_tuple("mmAnyTrue", TR::Double  , "10", "10", 1),
   std::make_tuple("mmAnyTrue", TR::Double  , "01", "01", 1),
   std::make_tuple("mmAnyTrue", TR::Double  , "11", "11", 1)
)));
INSTANTIATE_TEST_CASE_P(mmAnyFloat, ParameterizedUnaryMaskTest2, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<const char *, TR::DataTypes, const char *, const char *, int>>(
   std::make_tuple("mmAnyTrue", TR::Float  , "00", "00", 0),
   std::make_tuple("mmAnyTrue", TR::Float  , "00", "11", 0),
   std::make_tuple("mmAnyTrue", TR::Float  , "11", "00", 0),
   std::make_tuple("mmAnyTrue", TR::Float  , "10", "01", 0),
   std::make_tuple("mmAnyTrue", TR::Float  , "10", "10", 1),
   std::make_tuple("mmAnyTrue", TR::Float  , "01", "01", 1),
   std::make_tuple("mmAnyTrue", TR::Float  , "11", "11", 1)
)));
#define BOOST_TEST_MODULE compiler_tests tests
#include <boost/test/included/unit_test.hpp>

#include <compiler/compiler.h>

BOOST_AUTO_TEST_CASE(ArraysTest)
{
    std::string input =
        "int[5][4] a;"
        "int[5][4] b;"
        "int[5] c;"
        "x = a[b[i][j]][c[k]];"
    ;

    const std::string expectedCode =
        "t0 = i * 32\n"
        "t1 = j * 8\n"
        "t2 = t0 + t1\n"
        "t3 = b [t2]\n"
        "t4 = k * 8\n"
        "t5 = c [t4]\n"
        "t6 = t3 * 32\n"
        "t7 = t5 * 8\n"
        "t8 = t6 + t7\n"
        "t9 = a [t8]\n"
        "x = t9\n";

    const std::string threeAddressCode = Compile("grammar.csv", std::move(input));
    std::cout << threeAddressCode << std::endl;

    BOOST_TEST(threeAddressCode.c_str() == expectedCode.c_str());
}

BOOST_AUTO_TEST_CASE(SmokeTest)
{
    std::string input =
        "int a;"
        "a = 1 + 1 * 2;"
        "int[3][2] b;"
        "b[0][1] = 3;"
        "b[1][0] = 5;"
        "b[2][1] = 7;"
        "int c;"
        "c = b[0][0] + a;"
        "int[4][3][2] d;"
        "d[3][1][0] = 17;"
    ;

    const std::string expectedCode =
        "t0 = 1 * 2\n"
        "t1 = 1 + t0\n"
        "a = t1\n"
        "t2 = 0 * 16\n"
        "t3 = 1 * 8\n"
        "t4 = t2 + t3\n"
        "t5 = b [t4]\n"
        "t5 = 3\n"
        "t6 = 1 * 16\n"
        "t7 = 0 * 8\n"
        "t8 = t6 + t7\n"
        "t9 = b [t8]\n"
        "t9 = 5\n"
        "t10 = 2 * 16\n"
        "t11 = 1 * 8\n"
        "t12 = t10 + t11\n"
        "t13 = b [t12]\n"
        "t13 = 7\n"
        "t14 = 0 * 16\n"
        "t15 = 0 * 8\n"
        "t16 = t14 + t15\n"
        "t17 = b [t16]\n"
        "t18 = t17 + a\n"
        "c = t18\n"
        "t19 = 3 * 48\n"
        "t20 = 1 * 16\n"
        "t21 = 0 * 8\n"
        "t22 = t19 + t20\n"
        "t23 = t22 + t21\n"
        "t24 = d [t23]\n"
        "t24 = 17\n";

    const std::string threeAddressCode = Compile("grammar.csv", std::move(input));
    std::cout << threeAddressCode << std::endl;

    BOOST_TEST(threeAddressCode.c_str() == expectedCode.c_str());
}

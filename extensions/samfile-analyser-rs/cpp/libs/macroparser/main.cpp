#include "include/macroparser/macroparser.hpp"
#include <cassert>
#include <iostream>
#include <string.h>

#include <macroparser/macroparser.hpp>


int testcounter = 0;

#define RM_ANSI_FROM_COLORNAME 1
#include "colors.hpp"


#define NE "\n"

#define PRIN(MSG) std::cout << "\x1b[32m" << MSG << "\x1b[0m" << NE << NE;
#define PRINT(MSG) \
    std::cout << MSG << NE;

#define PRINT_SEPARATOR() \
    std::cout << "----------------------------------------" << NE;

#define ASSERT_PP(input, expected)                                      \
    do {                                                                \
        MacroParser parser;\
        std::cout << "Parsing Macros" NE; \
        std::string result = parser.parse_macros(input);                       \
                                                                        \
        PRINT_SEPARATOR();                                              \
        PRINT("Preprocessor Test");                                     \
        PRINT_SEPARATOR();                                              \
                                                                        \
        PRINT(GREEN "\nInput:" END);                                                \
        PRINT(input);                                                   \
                                                                        \
        PRINT(GREEN "\nExpected:" END);                                              \
        PRINT(expected);                                                \
                                                                        \
        PRINT(GREEN "\nGot:" END);                                                   \
        PRINT(result);                                                  \
                                                                        \
        if (strcmp(result.c_str(), expected) == 0)                      \
        {                                                               \
            PRINT("[PASS]");                                            \
        }                                                               \
        else                                                            \
        {                                                               \
            PRINT("[FAIL]");                                            \
        }                                                               \
                                                                        \
        assert(strcmp(result.c_str(), expected) == 0);                  \
                                                              \
                                                                        \
    } while(false);                                                     \
    PRIN("Works " << ++testcounter << " Tests");

#define assert_pp(input, expected) ASSERT_PP(input, expected)


// #define ASSERT_PP_FILE(filename, input, files, expected) \
//     do \
//     { \
//         char *result = preprocess_file(filename, input, files); \
//         assert(result != NULL); \
//         assert(strcmp(result, expected) == 0); \
//     } while (0)

// ============================================================================
// Testsuite fuer einen C++ Makro-Praeprozessor-Parser
// 300 Tests, generiert und verifiziert gegen g++ -E -P -std=c++20
//
// ANNAHMEN ueber assert_pp(input, expected):
//   - 'input' ist der komplette Quelltext (Makrodefinitionen + Verwendung),
//     Zeilen getrennt durch '\n'.
//   - 'expected' ist der vollstaendig expandierte/praeprozessierte Text,
//     mit entfernten fuehrenden/nachfolgenden Leerzeilen und ohne
//     Zeilenmarker (#line). Interne Leerzeilen (z.B. durch #define-Zeilen)
//     bleiben erhalten.
//   - Falls dein Parser anders normalisiert (z.B. alle Leerzeilen entfernt,
//     oder Whitespace pro Zeile trimmt), passe die Vergleichsfunktion in
//     assert_pp() entsprechend an, nicht die Erwartungswerte hier -- diese
//     wurden 1:1 aus einem echten Preprozessor (GCC) uebernommen.
//   - __LINE__, __COUNTER__ Tests pruefen nur das Zaehlverhalten/Format,
//     nicht absolute Werte deines Parsers falls diese abweichen.
// ============================================================================

#include <cassert>
#include <string>

// Vom Nutzer zu implementieren: fuehrt den eigenen Praeprozessor-Parser aus
// und vergleicht das Ergebnis mit expected.

void run_all_tests()
{
    // ------------------------------------------------------------------------
    // Objektartige Makros (einfache Werte)
    // ------------------------------------------------------------------------
    assert_pp("#define X 5\nX", "5");                 // #1
    assert_pp("#define X 42\nX", "42");               // #2
    assert_pp("#define X 0\nX", "0");                 // #3
    assert_pp("#define X -7\nX", "-7");               // #4
    assert_pp("#define X 3.14\nX", "3.14");           // #5
    assert_pp("#define X 'a'\nX", "'a'");             // #6
    assert_pp("#define X \"hello\"\nX", "\"hello\""); // #7
    assert_pp("#define X 1u\nX", "1u");               // #8
    assert_pp("#define X 1L\nX", "1L");               // #9
    assert_pp("#define X 0x1F\nX", "0x1F");           // #10
    assert_pp("#define X 010\nX", "010");             // #11
    assert_pp("#define X 1e5\nX", "1e5");             // #12
    assert_pp("#define X true\nX", "true");           // #13
    assert_pp("#define X false\nX", "false");         // #14
    assert_pp("#define X 100000000\nX", "100000000"); // #15
    assert_pp("#define X '\\n'\nX", "'\\n'");         // #16
    assert_pp("#define X 3.14159f\nX", "3.14159f");   // #17
    assert_pp("#define X -1\nX", "-1");               // #18
    assert_pp("#define X 0777\nX", "0777");           // #19
    assert_pp("#define X 0b101\nX", "0b101");         // #20
    assert_pp("#define X 999999\nX", "999999");       // #21
    assert_pp("#define X 1.0\nX", "1.0");             // #22
    assert_pp("#define X 2.5e-3\nX", "2.5e-3");       // #23

    // ------------------------------------------------------------------------
    // Argument-Prescan (Argumente werden vor Substitution expandiert)
    // ------------------------------------------------------------------------
    assert_pp("#define A 1\n#define ADD(x) (x+1)\nADD(A)", "(1+1)"); // #24
    assert_pp("#define B 2\n#define C 3\n#define ADD(x,y) (x+y)\nADD(B,C)",
              "(2+3)"); // #25
    assert_pp("#define TWO 2\n#define SQ(x) ((x)*(x))\nSQ(TWO)",
              "((2)*(2))"); // #26
    assert_pp("#define ONE 1\n#define TWO 2\n#define THREE 3\n#define "
              "SUM3(a,b,c) (a+b+c)\nSUM3(ONE,TWO,THREE)",
              "(1+2+3)"); // #27
    assert_pp("#define X 5\n#define Y X\n#define ADD(a,b) (a+b)\nADD(X,Y)",
              "(5+5)"); // #28
    assert_pp("#define VAL 10\n#define DOUBLE(x) ((x)*2)\nDOUBLE(VAL)",
              "((10)*2)");                                               // #29
    assert_pp("#define INNER 7\n#define OUTER(x) x\nOUTER(INNER)", "7"); // #30
    assert_pp(
        "#define A 1\n#define B A\n#define C B\n#define SHOW(x) x\nSHOW(C)",
        "1"); // #31
    assert_pp("#define N 3\n#define ARR_SIZE(x) [x]\nARR_SIZE(N)",
              "[3]"); // #32

    // ------------------------------------------------------------------------
    // Keine Argument-Expansion direkt neben # oder ##
    // ------------------------------------------------------------------------
    assert_pp("#define FIVE 5\n#define STR(x) #x\nSTR(FIVE)",
              "\"FIVE\""); // #33
    assert_pp("#define FIVE 5\n#define CAT(a,b) a##b\nCAT(FIVE,FIVE)",
              "FIVEFIVE"); // #34
    assert_pp("#define A 1\n#define B 2\n#define CAT(x,y) x##y\nCAT(A,B)",
              "AB"); // #35
    assert_pp("#define X 10\n#define STR(x) #x\n#define XSTR(x) STR(x)\nSTR(X) "
              "XSTR(X)",
              "\"X\" \"10\""); // #36
    assert_pp("#define VAL 5\n#define GLUE(a,b) a##b\n#define XGLUE(a,b) "
              "GLUE(a,b)\nGLUE(VAL,VAL) XGLUE(VAL,VAL)",
              "VALVAL 55");                                              // #37
    assert_pp("#define N 42\n#define RAWSTR(x) #x\nRAWSTR(N)", "\"N\""); // #38
    assert_pp(
        "#define A 9\n#define B 8\n#define RAW_CAT(x,y) x##y\nRAW_CAT(A,B)",
        "AB"); // #39

    // ------------------------------------------------------------------------
    // Diverse Grenzfaelle
    // ------------------------------------------------------------------------
    assert_pp("#define X\n#ifdef X\ndefined\n#endif", "defined"); // #40
    assert_pp("#define LPAREN (\n#define RPAREN )\nLPAREN 1 RPAREN",
              "( 1 )");                                                 // #41
    assert_pp("#define SEMI ;\nint x = 5 SEMI", "int x = 5 ;");         // #42
    assert_pp("#define TRUE 1\n#define FALSE 0\nTRUE FALSE", "1 0");    // #43
    assert_pp("#define NULL_PTR 0\nint* p = NULL_PTR;", "int* p = 0;"); // #44
    assert_pp("#define BEGIN {\n#define END }\nBEGIN x++; END",
              "{ x++; }"); // #45
    assert_pp("#define CONST_VAL const int x = 5;\nCONST_VAL",
              "const int x = 5;"); // #46
    assert_pp("#define ARRSIZE(a) (sizeof(a)/sizeof((a)[0]))\nARRSIZE(myarr)",
              "(sizeof(myarr)/sizeof((myarr)[0]))"); // #47
    assert_pp("#define ONE_LINE_IF(c,s) if(c) s;\nONE_LINE_IF(x>0,y=1)",
              "if(x>0) y=1;");                                      // #48
    assert_pp("#define BIT(n) (1<<(n))\nBIT(3)", "(1<<(3))");       // #49
    assert_pp("#define KB(n) ((n)*1024)\nKB(4)", "((4)*1024)");     // #50
    assert_pp("#define MB(n) (KB(n)*1024)\nMB(2)", "(KB(2)*1024)"); // #51
    assert_pp("#define STATIC_ASSERT(c) "
              "static_assert(c)\nSTATIC_ASSERT(sizeof(int)==4)",
              "static_assert(sizeof(int)==4)"); // #52
    assert_pp(
        "#define FOREACH(item,list) for(auto item : list)\nFOREACH(x,vec)",
        "for(auto x : vec)"); // #53
    assert_pp("#define UNUSED_VAR(x) (void)(x)\nUNUSED_VAR(y)",
              "(void)(y)"); // #54
    assert_pp(
        "#define MIN_MAX(a,b) MIN(a,b), MAX(a,b)\n#define MIN(a,b) "
        "((a)<(b)?(a):(b))\n#define MAX(a,b) ((a)>(b)?(a):(b))\nMIN_MAX(3,7)",
        "((3)<(7)?(3):(7)), ((3)>(7)?(3):(7))"); // #55
    assert_pp("#define VERSION_MAJOR 1\n#define VERSION_MINOR 2\n#define "
              "VERSION_PATCH 3\n#define VERSION_STR(a,b,c) #a \".\" #b \".\" "
              "#c\n#define VERSION_STR2(a,b,c) "
              "VERSION_STR(a,b,c)\nVERSION_STR2(VERSION_MAJOR,VERSION_MINOR,"
              "VERSION_PATCH)",
              "\"1\" \".\" \"2\" \".\" \"3\""); // #56

    // ------------------------------------------------------------------------
    // Verkettete objektartige Makros (Makro referenziert Makro)
    // ------------------------------------------------------------------------
    assert_pp("#define A 1\n#define B A\n#define C B\nC", "1"); // #57
    assert_pp("#define A 1\n#define B (A+1)\n#define C (B+1)\nC",
              "((1+1)+1)"); // #58
    assert_pp("#define ONE 1\n#define TWO 2\n#define THREE (ONE+TWO)\nTHREE",
              "(1+2)");                                               // #59
    assert_pp("#define PI 3\n#define TWO_PI (2*PI)\nTWO_PI", "(2*3)"); // #60
    assert_pp("#define BASE 10\n#define DOUBLE (BASE*2)\n#define QUAD "
              "(DOUBLE*2)\nQUAD",
              "((10*2)*2)");                                      // #61
    assert_pp("#define A B\n#define B C\n#define C 42\nA", "42"); // #62
    assert_pp("#define X 1\n#define Y X\n#define Z Y\nZ", "1");   // #63
    assert_pp("#define N 5\n#define M N\nM M", "5 5");            // #64
    assert_pp("#define A 1\n#define B 2\nA B", "1 2");            // #65
    assert_pp("#define GREETING \"hi\"\n#define MSG GREETING\nMSG",
              "\"hi\""); // #66
    assert_pp("#define UNIT 1\n#define KILO (UNIT*1000)\nKILO",
              "(1*1000)");                                                // #67
    assert_pp("#define A(x) x\n#define B A\nB(5)", "5");                  // #68
    assert_pp("#define X 10\n#define Y (X)\n#define Z (Y)\nZ", "((10))"); // #69

    // ------------------------------------------------------------------------
    // Funktionsartige Makros mit 1 Argument
    // ------------------------------------------------------------------------
    assert_pp("#define SQUARE(x) ((x)*(x))\nSQUARE(5)", "((5)*(5))");     // #70
    assert_pp("#define NEG(x) (-(x))\nNEG(3)", "(-(3))");                 // #71
    assert_pp("#define DOUBLEIT(x) ((x)+(x))\nDOUBLEIT(4)", "((4)+(4))"); // #72
    assert_pp("#define IDENT(x) x\nIDENT(hello)", "hello");               // #73
    assert_pp("#define PAREN(x) (x)\nPAREN(1+2)", "(1+2)");               // #74
    assert_pp("#define INC(x) ((x)+1)\nINC(9)", "((9)+1)");               // #75
    assert_pp("#define ABS(x) ((x)<0?-(x):(x))\nABS(-5)",
              "((-5)<0?-(-5):(-5))");                             // #76
    assert_pp("#define ZERO_OF(x) 0\nZERO_OF(anything)", "0");    // #77
    assert_pp("#define TRIPLE(x) ((x)*3)\nTRIPLE(7)", "((7)*3)"); // #78
    assert_pp("#define WRAP(x) [x]\nWRAP(42)", "[42]");           // #79
    assert_pp("#define CALL(f) f()\nCALL(foo)", "foo()");         // #80
    assert_pp("#define SIZEOF_LIKE(x) sizeof(x)\nSIZEOF_LIKE(int)",
              "sizeof(int)");                                         // #81
    assert_pp("#define CAST(x) (int)(x)\nCAST(3.14)", "(int)(3.14)"); // #82

    // ------------------------------------------------------------------------
    // Funktionsartige Makros mit 2 Argumenten
    // ------------------------------------------------------------------------
    assert_pp("#define ADD(a,b) ((a)+(b))\nADD(2,3)", "((2)+(3))");   // #83
    assert_pp("#define SUB(a,b) ((a)-(b))\nSUB(10,4)", "((10)-(4))"); // #84
    assert_pp("#define MUL(a,b) ((a)*(b))\nMUL(6,7)", "((6)*(7))");   // #85
    assert_pp("#define MAX(a,b) ((a)>(b)?(a):(b))\nMAX(3,9)",
              "((3)>(9)?(3):(9))"); // #86
    assert_pp("#define MIN(a,b) ((a)<(b)?(a):(b))\nMIN(3,9)",
              "((3)<(9)?(3):(9))");                                   // #87
    assert_pp("#define SWAP(a,b) b, a\nSWAP(1,2)", "2, 1");           // #88
    assert_pp("#define PAIR(a,b) {a, b}\nPAIR(1,2)", "{1, 2}");       // #89
    assert_pp("#define DIV(a,b) ((a)/(b))\nDIV(10,2)", "((10)/(2))"); // #90
    assert_pp("#define MOD(a,b) ((a)%(b))\nMOD(10,3)", "((10)%(3))"); // #91
    assert_pp("#define AND(a,b) ((a)&&(b))\nAND(1,0)", "((1)&&(0))"); // #92
    assert_pp("#define OR(a,b) ((a)||(b))\nOR(0,1)", "((0)||(1))");   // #93
    assert_pp("#define EQ(a,b) ((a)==(b))\nEQ(5,5)", "((5)==(5))");   // #94
    assert_pp("#define CONCAT_SPACE(a,b) a b\nCONCAT_SPACE(int,x)",
              "int x"); // #95

    // ------------------------------------------------------------------------
    // Funktionsartige Makros mit 3+ Argumenten
    // ------------------------------------------------------------------------
    assert_pp("#define ADD3(a,b,c) ((a)+(b)+(c))\nADD3(1,2,3)",
              "((1)+(2)+(3))"); // #96
    assert_pp("#define CLAMP(x,lo,hi) "
              "((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))\nCLAMP(15,0,10)",
              "((15)<(0)?(0):((15)>(10)?(10):(15)))"); // #97
    assert_pp("#define TERNARY(c,a,b) ((c)?(a):(b))\nTERNARY(1,10,20)",
              "((1)?(10):(20))"); // #98
    assert_pp("#define SUM4(a,b,c,d) ((a)+(b)+(c)+(d))\nSUM4(1,2,3,4)",
              "((1)+(2)+(3)+(4))"); // #99
    assert_pp("#define MAKE_RGB(r,g,b) {r,g,b}\nMAKE_RGB(255,0,0)",
              "{255,0,0}"); // #100
    assert_pp("#define VOL(l,w,h) ((l)*(w)*(h))\nVOL(2,3,4)",
              "((2)*(3)*(4))"); // #101
    assert_pp("#define FORLOOP(i,n,body) for(int "
              "i=0;i<n;i++){body}\nFORLOOP(i,10,x++)",
              "for(int i=0;i<10;i++){x++}"); // #102
    assert_pp("#define FIVEARGS(a,b,c,d,e) a+b+c+d+e\nFIVEARGS(1,2,3,4,5)",
              "1+2+3+4+5"); // #103

    // ------------------------------------------------------------------------
    // Mehrfachverwendung eines Arguments im Makrokoerper
    // ------------------------------------------------------------------------
    assert_pp("#define SQR(x) ((x)*(x))\nSQR(a+b)", "((a+b)*(a+b))"); // #104
    assert_pp("#define TWICE(x) x x\nTWICE(hi)", "hi hi");            // #105
    assert_pp("#define THRICE(x) x x x\nTHRICE(y)", "y y y");         // #106
    assert_pp("#define SELFADD(x) ((x)+(x)+(x))\nSELFADD(2)",
              "((2)+(2)+(2))");                                          // #107
    assert_pp("#define ECHO4(x) x x x x\nECHO4(z)", "z z z z");          // #108
    assert_pp("#define WRAP2(x) (x)(x)\nWRAP2(f)", "(f)(f)");            // #109
    assert_pp("#define DUP_STR(x) #x #x\nDUP_STR(hi)", "\"hi\" \"hi\""); // #110
    assert_pp("#define SUMSAME(x) (x+x+x+x+x)\nSUMSAME(1)",
              "(1+1+1+1+1)"); // #111

    // ------------------------------------------------------------------------
    // Verschachtelte Makroaufrufe
    // ------------------------------------------------------------------------
    assert_pp(
        "#define SQUARE(x) ((x)*(x))\n#define CUBE(x) (SQUARE(x)*(x))\nCUBE(3)",
        "(((3)*(3))*(3))"); // #112
    assert_pp("#define ADD(a,b) ((a)+(b))\n#define ADD3(a,b,c) "
              "ADD(ADD(a,b),c)\nADD3(1,2,3)",
              "((((1)+(2)))+(3))"); // #113
    assert_pp("#define INC(x) ((x)+1)\nINC(INC(INC(0)))",
              "((((((0)+1))+1))+1)"); // #114
    assert_pp(
        "#define DOUBLE(x) ((x)*2)\n#define QUAD(x) DOUBLE(DOUBLE(x))\nQUAD(5)",
        "((((5)*2))*2)"); // #115
    assert_pp("#define MAX(a,b) ((a)>(b)?(a):(b))\n#define MAX3(a,b,c) "
              "MAX(MAX(a,b),c)\nMAX3(1,9,4)",
              "((((1)>(9)?(1):(9)))>(4)?(((1)>(9)?(1):(9))):(4))");   // #116
    assert_pp("#define NEG(x) (-(x))\nNEG(NEG(5))", "(-((-(5))))");   // #117
    assert_pp("#define WRAP(x) (x)\nWRAP(WRAP(WRAP(1)))", "(((1)))"); // #118
    assert_pp("#define ADD(a,b) ((a)+(b))\n#define MUL(a,b) "
              "((a)*(b))\nMUL(ADD(1,2),ADD(3,4))",
              "((((1)+(2)))*(((3)+(4))))"); // #119
    assert_pp("#define F(x) G(x)\n#define G(x) ((x)+1)\nF(10)",
              "((10)+1)"); // #120
    assert_pp("#define A(x) B(x)+1\n#define B(x) C(x)+1\n#define C(x) x\nA(1)",
              "1+1+1"); // #121
    assert_pp(
        "#define SQ(x) ((x)*(x))\n#define SUMSQ(a,b) (SQ(a)+SQ(b))\nSUMSQ(3,4)",
        "(((3)*(3))+((4)*(4)))");                           // #122
    assert_pp("#define ID(x) x\nID(ID(ID(ID(42))))", "42"); // #123
    assert_pp(
        "#define TIMES2(x) ((x)*2)\n#define TIMES4(x) "
        "TIMES2(TIMES2(x))\n#define TIMES8(x) TIMES2(TIMES4(x))\nTIMES8(1)",
        "((((((1)*2))*2))*2)"); // #124

    // ------------------------------------------------------------------------
    // Rekursive/selbstreferenzierende Makros (Blue Paint / Selbstschutz)
    // ------------------------------------------------------------------------
    assert_pp("#define A A\nA", "A");                      // #125
    assert_pp("#define X (X+1)\nX", "(X+1)");              // #126
    assert_pp("#define A B\n#define B A\nA", "A");         // #127
    assert_pp("#define F(x) F(x)\nF(1)", "F(1)");          // #128
    assert_pp("#define A A B\n#define B B A\nA", "A B A"); // #129
    assert_pp("#define FACT(n) n * FACT(n-1)\nFACT(5)",
              "5 * FACT(5 -1)");                      // #130
    assert_pp("#define X X X\nX", "X X");             // #131
    assert_pp("#define A(x) A(x)+1\nA(1)", "A(1)+1"); // #132

    // ------------------------------------------------------------------------
    // Stringizing-Operator (#)
    // ------------------------------------------------------------------------
    assert_pp("#define STR(x) #x\nSTR(hello)", "\"hello\"");             // #133
    assert_pp("#define STR(x) #x\nSTR(hello world)", "\"hello world\""); // #134
    assert_pp("#define STR(x) #x\nSTR(123)", "\"123\"");                 // #135
    assert_pp("#define STR(x) #x\nSTR(a+b)", "\"a+b\"");                 // #136
    assert_pp("#define STR(x) #x\nSTR()", "\"\"");                       // #137
    assert_pp("#define STR(x) #x\nSTR(  spaced   out  )",
              "\"spaced out\""); // #138
    assert_pp("#define STR(x) #x\nSTR(\"already quoted\")",
              "\"\\\"already quoted\\\"\"");             // #139
    assert_pp("#define STR(x) #x\nSTR('c')", "\"'c'\""); // #140
    assert_pp("#define STR2(x) #x\n#define STR(x) STR2(x)\n#define X 5\nSTR(X)",
              "\"5\"");                                          // #141
    assert_pp("#define STR(x) #x\nSTR(a b c d)", "\"a b c d\""); // #142
    assert_pp("#define STR(x) #x\nSTR(if(x)return;)",
              "\"if(x)return;\"");                         // #143
    assert_pp("#define STR(x) #x\nSTR(x==y)", "\"x==y\""); // #144
    assert_pp("#define MKSTR(a,b) #a #b\nMKSTR(hello,world)",
              "\"hello\" \"world\"");                                  // #145
    assert_pp("#define STR(x) #x\nSTR(-1)", "\"-1\"");                 // #146
    assert_pp("#define STR(x) #x\nSTR(a\\b)", "\"a\\b\"");             // #147
    assert_pp("#define STR(x) #x\nSTR(<vector>)", "\"<vector>\"");     // #148
    assert_pp("#define STR(x) #x\nSTR(a==b&&c!=d)", "\"a==b&&c!=d\""); // #149

    // ------------------------------------------------------------------------
    // Token-Pasting-Operator (##)
    // ------------------------------------------------------------------------
    assert_pp("#define CAT(a,b) a##b\nCAT(foo,bar)", "foobar");   // #150
    assert_pp("#define CAT(a,b) a##b\nCAT(1,2)", "12");           // #151
    assert_pp("#define CAT(a,b) a##b\nCAT(x,1)", "x1");           // #152
    assert_pp("#define CAT3(a,b,c) a##b##c\nCAT3(x,y,z)", "xyz"); // #153
    assert_pp("#define MKID(n) var##n\nMKID(1) MKID(2) MKID(3)",
              "var1 var2 var3"); // #154
    assert_pp("#define CAT(a,b) a##b\n#define X 1\n#define Y 2\nCAT(X,Y)",
              "XY"); // #155
    assert_pp("#define EXPAND_CAT(a,b) a##b\n#define CAT(a,b) "
              "EXPAND_CAT(a,b)\n#define X 1\nCAT(X,X)",
              "11");                                              // #156
    assert_pp("#define PREFIX(x) pre##x\nPREFIX(fix)", "prefix"); // #157
    assert_pp("#define SUFFIX(x) x##_suffix\nSUFFIX(name)",
              "name_suffix");                                         // #158
    assert_pp("#define JOIN(a,b) a##_##b\nJOIN(foo,bar)", "foo_bar"); // #159
    assert_pp("#define CAT(a,b) a##b\nCAT(,x)", "x");                 // #160
    assert_pp("#define CAT(a,b) a##b\nCAT(x,)", "x");                 // #161
    assert_pp("#define GLUE(a,b) a ## b\nGLUE(0x,FF)", "0xFF");       // #162
    assert_pp("#define CAT(a,b) a##b\nCAT(int,32_t)", "int32_t");     // #163
    assert_pp("#define FUNC_NAME(n) func_##n\n#define CALL_FUNC(n) "
              "FUNC_NAME(n)()\nCALL_FUNC(1)",
              "func_1()");                                          // #164
    assert_pp("#define MKARR(n) arr##n[10]\nMKARR(A)", "arrA[10]"); // #165
    assert_pp("#define CAT(a,b) a##b\nCAT(L,\"str\")", "L\"str\""); // #166

    // ------------------------------------------------------------------------
    // Kombination aus # und ##
    // ------------------------------------------------------------------------
    assert_pp("#define STR(x) #x\n#define CAT(a,b) a##b\nSTR(CAT(foo,bar))",
              "\"CAT(foo,bar)\""); // #167
    assert_pp("#define XSTR(x) STR(x)\n#define STR(x) #x\n#define CAT(a,b) "
              "a##b\nXSTR(CAT(1,2))",
              "\"12\""); // #168
    assert_pp("#define MK_NAME_STR(a,b) #a \"_\" #b\nMK_NAME_STR(foo,bar)",
              "\"foo\" \"_\" \"bar\"");                              // #169
    assert_pp("#define CAT_STR(a,b) #a##b\nCAT_STR(x,y)", "\"x\"y"); // #170
    assert_pp(
        "#define LOG(name,val) printf(#name \"=%d\\n\", val)\nLOG(counter,5)",
        "printf(\"counter\" \"=%d\\n\", 5)"); // #171
    assert_pp("#define VAR(n) v##n\n#define VARSTR(n) #n\nVAR(1) VARSTR(1)",
              "v1 \"1\""); // #172
    assert_pp("#define A(x,y) x##y\n#define B(x,y) #x #y\nA(1,2) B(1,2)",
              "12 \"1\" \"2\""); // #173
    assert_pp("#define CONCAT(a,b) a##b\n#define TOSTR(a) #a\n#define "
              "TOSTR2(a) TOSTR(a)\nTOSTR2(CONCAT(hello,world))",
              "\"helloworld\""); // #174

    // ------------------------------------------------------------------------
    // Variadische Makros (__VA_ARGS__)
    // ------------------------------------------------------------------------
    assert_pp(
        "#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)\nLOG(\"%d %d\", 1, 2)",
        "printf(\"%d %d\", 1, 2)");                                 // #175
    assert_pp("#define SUM(...) __VA_ARGS__\nSUM(1,2,3)", "1,2,3"); // #176
    assert_pp("#define FIRST(a, ...) a\nFIRST(1,2,3)", "1");        // #177
    assert_pp("#define WRAP(...) (__VA_ARGS__)\nWRAP(1,2,3)",
              "(1,2,3)"); // #178
    assert_pp(
        "#define DEBUG(...) fprintf(stderr, __VA_ARGS__)\nDEBUG(\"x=%d\", x)",
        "fprintf(stderr, \"x=%d\", x)"); // #179
    assert_pp("#define CALL(f, ...) f(__VA_ARGS__)\nCALL(foo, 1, 2, 3)",
              "foo(1, 2, 3)"); // #180
    assert_pp("#define TWO_PLUS(a, b, ...) a+b\nTWO_PLUS(1,2,3,4,5)",
              "1 +2"); // #181
    assert_pp("#define VARARGS_COUNT(...) #__VA_ARGS__\nVARARGS_COUNT(a,b,c)",
              "\"a,b,c\""); // #182
    assert_pp("#define PRINTF_LIKE(fmt, ...) printf(fmt, "
              "##__VA_ARGS__)\nPRINTF_LIKE(\"hi\")",
              "printf(\"hi\")"); // #183
    assert_pp("#define MACRO(a, b, ...) a + b + (__VA_ARGS__)\nMACRO(1,2,3,4)",
              "1 + 2 + (3,4)"); // #184
    assert_pp("#define LIST(...) {__VA_ARGS__}\nLIST(1,2,3,4,5)",
              "{1,2,3,4,5}"); // #185
    assert_pp("#define APPLY(f,...) f(__VA_ARGS__)\nAPPLY(sum,1,2)",
              "sum(1,2)"); // #186
    assert_pp("#define EAT_FIRST(a,...) __VA_ARGS__\nEAT_FIRST(x,y,z)",
              "y,z"); // #187

    // ------------------------------------------------------------------------
    // __VA_OPT__ (C++20)
    // ------------------------------------------------------------------------
    assert_pp("#define LOG(fmt, ...) printf(fmt __VA_OPT__(,) "
              "__VA_ARGS__)\nLOG(\"hi\")",
              "printf(\"hi\" )"); // #188
    assert_pp("#define LOG(fmt, ...) printf(fmt __VA_OPT__(,) "
              "__VA_ARGS__)\nLOG(\"hi\", 1, 2)",
              "printf(\"hi\" , 1, 2)");                                 // #189
    assert_pp("#define F(...) __VA_OPT__(has args)\nF()", "");          // #190
    assert_pp("#define F(...) __VA_OPT__(has args)\nF(1)", "has args"); // #191
    assert_pp("#define CALL(f,...) f(__VA_OPT__(__VA_ARGS__))\nCALL(foo)",
              "foo()"); // #192
    assert_pp("#define CALL(f,...) f(__VA_OPT__(__VA_ARGS__))\nCALL(foo,1,2)",
              "foo(1,2)");                                        // #193
    assert_pp("#define COUNT(...) __VA_OPT__(1)\nCOUNT()", "");   // #194
    assert_pp("#define COUNT(...) __VA_OPT__(1)\nCOUNT(x)", "1"); // #195

    // ------------------------------------------------------------------------
    // #undef und Neudefinition
    // ------------------------------------------------------------------------
    assert_pp("#define X 1\n#undef X\n#define X 2\nX", "2"); // #196
    assert_pp("#define X 1\nX\n#undef X\n#ifdef "
              "X\nDEFINED\n#else\nNOTDEFINED\n#endif",
              "1\nNOTDEFINED");                              // #197
    assert_pp("#define A 1\n#define B 2\n#undef A\nB", "2"); // #198
    assert_pp("#define F(x) x\n#undef F\n#define F(x) x+1\nF(5)",
              "5 +1"); // #199
    assert_pp("#define X 5\n#undef X\n#undef X\n#ifndef X\nOK\n#endif",
              "OK"); // #200
    assert_pp("#define TEMP 1\n#undef TEMP\n#define TEMP 2\n#undef "
              "TEMP\n#define TEMP 3\nTEMP",
              "3");                                                // #201
    assert_pp("#define A 1\nA\n#undef A\n#define A 2\nA", "1\n2"); // #202
    assert_pp("#undef NEVER_DEFINED\nok", "ok");                   // #203

    // ------------------------------------------------------------------------
    // #ifdef / #ifndef
    // ------------------------------------------------------------------------
    assert_pp("#define DEBUG\n#ifdef DEBUG\nyes\n#endif", "yes"); // #204
    assert_pp("#ifdef NOTDEFINED\nyes\n#else\nno\n#endif", "no"); // #205
    assert_pp("#define FEATURE\n#ifndef FEATURE\nno\n#else\nyes\n#endif",
              "yes"); // #206
    assert_pp("#ifndef GUARD\n#define GUARD\ncontent\n#endif",
              "content"); // #207
    assert_pp("#define A\n#ifdef A\n#ifdef B\nab\n#else\naonly\n#endif\n#endif",
              "aonly"); // #208
    assert_pp("#define A\n#define B\n#ifdef A\n#ifdef B\nab\n#endif\n#endif",
              "ab"); // #209
    assert_pp("#ifdef X\na\n#elif defined(Y)\nb\n#else\nc\n#endif",
              "c"); // #210
    assert_pp("#define Y\n#ifdef X\na\n#elif defined(Y)\nb\n#else\nc\n#endif",
              "b"); // #211
    assert_pp("#define VERBOSE\n#ifdef VERBOSE\nlog(1);\n#endif\n#ifndef "
              "QUIET\nlog(2);\n#endif",
              "log(1);\nlog(2);"); // #212
    assert_pp("#ifndef HEADER_H\n#define HEADER_H\ntype x;\n#endif",
              "type x;"); // #213
    assert_pp("#define WIN\n#ifdef WIN\nwindows\n#elif "
              "defined(LINUX)\nlinux\n#else\nother\n#endif",
              "windows"); // #214
    assert_pp("#define LINUX\n#ifdef WIN\nwindows\n#elif "
              "defined(LINUX)\nlinux\n#else\nother\n#endif",
              "linux"); // #215
    assert_pp("#ifdef WIN\nwindows\n#elif "
              "defined(LINUX)\nlinux\n#else\nother\n#endif",
              "other"); // #216

    // ------------------------------------------------------------------------
    // #if / #elif / #else mit konstanten Ausdruecken
    // ------------------------------------------------------------------------
    assert_pp("#if 1\nyes\n#endif", "yes");           // #217
    assert_pp("#if 0\nyes\n#else\nno\n#endif", "no"); // #218
    assert_pp("#define X 5\n#if X > 3\nbig\n#else\nsmall\n#endif",
              "big"); // #219
    assert_pp("#define X 2\n#if X > 3\nbig\n#else\nsmall\n#endif",
              "small"); // #220
    assert_pp("#define VER 3\n#if VER == 1\none\n#elif VER == 2\ntwo\n#elif "
              "VER == 3\nthree\n#else\nother\n#endif",
              "three");                                      // #221
    assert_pp("#if 1 + 1 == 2\nmath_ok\n#endif", "math_ok"); // #222
    assert_pp("#if (1 && 1)\nboth\n#endif", "both");         // #223
    assert_pp("#if (1 || 0)\neither\n#endif", "either");     // #224
    assert_pp("#if !0\nnotzero\n#endif", "notzero");         // #225
    assert_pp(
        "#define A 1\n#define B 0\n#if A && B\nboth\n#else\nnotboth\n#endif",
        "notboth"); // #226
    assert_pp(
        "#define A 1\n#define B 0\n#if A || B\neither\n#else\nneither\n#endif",
        "either");                                                   // #227
    assert_pp("#if 10 % 3 == 1\nmodworks\n#endif", "modworks");      // #228
    assert_pp("#if (5 > 3) && (2 < 4)\nrangeok\n#endif", "rangeok"); // #229
    assert_pp("#define LEVEL 2\n#if LEVEL >= 1 && LEVEL <= 3\ninrange\n#endif",
              "inrange");                                     // #230
    assert_pp("#if 1 << 3 == 8\nshiftok\n#endif", "shiftok"); // #231
    assert_pp("#define FLAGS 0x03\n#if FLAGS & 0x01\nbit0set\n#endif",
              "bit0set"); // #232
    assert_pp("#define A 5\n#define B 10\n#if A + B == 15\nsumok\n#endif",
              "sumok");                                            // #233
    assert_pp("#if defined(__cplusplus)\niscpp\n#endif", "iscpp"); // #234

    // ------------------------------------------------------------------------
    // defined()-Operator
    // ------------------------------------------------------------------------
    assert_pp("#define A\n#if defined(A)\nyes\n#endif", "yes"); // #235
    assert_pp("#if defined(B)\nyes\n#else\nno\n#endif", "no");  // #236
    assert_pp("#define A\n#if defined A\nyes\n#endif", "yes");  // #237
    assert_pp(
        "#define A\n#define B\n#if defined(A) && defined(B)\nboth\n#endif",
        "both"); // #238
    assert_pp(
        "#define A\n#if defined(A) && defined(B)\nboth\n#else\nnotboth\n#endif",
        "notboth");                                              // #239
    assert_pp("#define A\n#if !defined(B)\nnob\n#endif", "nob"); // #240
    assert_pp("#if defined(A) || defined(B)\nyes\n#else\nno\n#endif",
              "no"); // #241
    assert_pp("#define A\n#if defined(A) || defined(B)\nyes\n#else\nno\n#endif",
              "yes"); // #242

    // ------------------------------------------------------------------------
    // Leere Makrokoerper
    // ------------------------------------------------------------------------
    assert_pp("#define EMPTY\nbefore EMPTY after", "before after"); // #243
    assert_pp("#define EMPTY()\nEMPTY()", "");                      // #244
    assert_pp("#define NOTHING\nx NOTHING y", "x y");               // #245
    assert_pp("#define EMPTY\nEMPTY EMPTY EMPTY", "");              // #246
    assert_pp("#define UNUSED(x)\nUNUSED(var);", ";");              // #247
    assert_pp("#define NOOP()\nNOOP(); NOOP();", "; ;");            // #248
    assert_pp("#define BLANK\nstart[BLANK]end", "start[]end");      // #249

    // ------------------------------------------------------------------------
    // Mehrzeilige Makros (Backslash-Fortsetzung)
    // ------------------------------------------------------------------------
    assert_pp("#define ADD(a,b) \\\n  ((a) + (b))\nADD(1,2)",
              "((1) + (2))"); // #250
    assert_pp("#define BIG_MACRO(x) \\\n  do { \\\n    x; \\\n  } "
              "while(0)\nBIG_MACRO(foo())",
              "do { foo(); } while(0)");                          // #251
    assert_pp("#define LONG_VALUE \\\n  100\nLONG_VALUE", "100"); // #252
    assert_pp("#define MULTI(a,b,c) \\\n  a + \\\n  b + \\\n  c\nMULTI(1,2,3)",
              "1 + 2 + 3"); // #253
    assert_pp("#define SWAP(a,b) \\\n  { \\\n    auto t = a; \\\n    a = b; "
              "\\\n    b = t; \\\n  }\nSWAP(x,y)",
              "{ auto t = x; x = y; y = t; }"); // #254
    assert_pp("#define CHAIN \\\n  1 + \\\n  2 + \\\n  3\nCHAIN",
              "1 + 2 + 3"); // #255
    assert_pp("#define ASSERT(cond) \\\n  if (!(cond)) \\\n    "
              "fail();\nASSERT(x > 0)",
              "if (!(x > 0)) fail();");                         // #256
    assert_pp("#define TABLE \\\n  {1,2,3}\nTABLE", "{1,2,3}"); // #257

    // ------------------------------------------------------------------------
    // Kommentare in/um Makrodefinitionen
    // ------------------------------------------------------------------------
    assert_pp("#define X /* comment */ 5\nX", "5"); // #258
    assert_pp("#define ADD(a,b) (a /* plus */ + b)\nADD(1,2)",
              "(1 + 2)");                                      // #259
    assert_pp("// leading comment\n#define X 5\nX", "5");      // #260
    assert_pp("#define X 5 // trailing comment\nX", "5");      // #261
    assert_pp("#define /* weird */ X 5\nX", "5");              // #262
    assert_pp("#define F(x /* the arg */) x\nF(1)", "1");      // #263
    assert_pp("/* block\n   comment */\n#define X 1\nX", "1"); // #264

    // ------------------------------------------------------------------------
    // __LINE__
    // ------------------------------------------------------------------------
    assert_pp("__LINE__", "1");                             // #265
    assert_pp("\n\n__LINE__", "3");                         // #266
    assert_pp("#define WHERE __LINE__\nWHERE", "2");        // #267
    assert_pp("line1\nline2\n__LINE__", "line1\nline2\n3"); // #268
    assert_pp("#define MARK() __LINE__\nMARK()", "2");      // #269

    // ------------------------------------------------------------------------
    // __COUNTER__
    // ------------------------------------------------------------------------
    assert_pp("__COUNTER__", "0");                              // #270
    assert_pp("__COUNTER__ __COUNTER__", "0 1");                // #271
    assert_pp("__COUNTER__ __COUNTER__ __COUNTER__", "0 1 2");  // #272
    assert_pp("#define UID() __COUNTER__\nUID() UID()", "0 1"); // #273
    assert_pp("#define CAT(a,b) a##b\n#define CAT2(a,b) CAT(a,b)\n#define UNIQ "
              "CAT2(id_,__COUNTER__)\nUNIQ UNIQ",
              "id_0 id_1"); // #274

    // ------------------------------------------------------------------------
    // Whitespace-Handhabung in Makroargumenten
    // ------------------------------------------------------------------------
    assert_pp("#define ADD(a,b) ((a)+(b))\nADD(  1  ,   2   )",
              "((1)+(2))");                                             // #275
    assert_pp("#define F(x)   x  \nF(hello)", "hello");                 // #276
    assert_pp("#define   X    5\nX", "5");                              // #277
    assert_pp("#define ADD(a,b) a+b\nADD(1,\n2)", "1 +2");              // #278
    assert_pp("#define F(x) x\nF(\n  value\n)", "value");               // #279
    assert_pp("#define G(a,   b,c) a+b+c\nG(1,2,3)", "1 +2 +3");        // #280
    assert_pp("#define TABS(a,b)\ta+b\nTABS(1,2)", "1 +2");             // #281
    assert_pp("#define SPACED (   1   +   2   )\nSPACED", "( 1 + 2 )"); // #282

    // ------------------------------------------------------------------------
    // Makro expandiert zu Makronamen (Rescanning)
    // ------------------------------------------------------------------------
    assert_pp("#define FOO BAR\n#define BAR 42\nFOO", "42");       // #283
    assert_pp("#define A(x) B(x)\n#define B(x) x*2\nA(5)", "5*2"); // #284
    assert_pp("#define CALL_IT FUNC()\n#define FUNC() 99\nCALL_IT",
              "99"); // #285
    assert_pp(
        "#define GET_MAX MAX\n#define MAX(a,b) ((a)>(b)?(a):(b))\nGET_MAX(1,2)",
        "((1)>(2)?(1):(2))"); // #286
    assert_pp("#define APPLY_F F\n#define F(x) x+1\nAPPLY_F(10)",
              "10 +1"); // #287
    assert_pp("#define NAME FUNC_NAME\n#define FUNC_NAME(x) x\nNAME(hi)",
              "hi"); // #288
    assert_pp(
        "#define INDIRECT(x) x\n#define REAL(x) ((x)*2)\nINDIRECT(REAL)(5)",
        "((5)*2)"); // #289
    assert_pp("#define TABLE_ENTRY ENTRY\n#define ENTRY(x) {x}\nTABLE_ENTRY(1)",
              "{1}"); // #290
    assert_pp("#define OP PLUS\n#define PLUS(a,b) a+b\nOP(1,2)",
              "1 +2"); // #291

    // ------------------------------------------------------------------------
    // Funktionsartige Makros ohne Argumente FOO()
    // ------------------------------------------------------------------------
    assert_pp("#define FOO() 42\nFOO()", "42");                   // #292
    assert_pp("#define GREET() \"hello\"\nGREET()", "\"hello\""); // #293
    assert_pp("#define NOARG() (1+1)\nNOARG()", "(1+1)");         // #294
    assert_pp("#define GETX() x\nGETX()", "x");                   // #295
    assert_pp("#define CALLBACK() do_something()\nCALLBACK();",
              "do_something();");                                       // #296
    assert_pp("#define VERSION() \"1.0\"\nVERSION()", "\"1.0\"");       // #297
    assert_pp("#define INIT() {0}\nauto v = INIT();", "auto v = {0};"); // #298
    assert_pp("#define TIMESTAMP() 0\nTIMESTAMP() TIMESTAMP()", "0 0"); // #299
    assert_pp("#define NOOP_FN() ;\nNOOP_FN()", ";");                   // #300
}


int main()
{
    std::cout << "injecting main" NE;

    // Test Assert
    // assert(0);

    // Test the Macro Parser here

    run_all_tests();
    printf("Alle 300 Tests bestanden!\n");

    assert_pp("#define X 5\nX", "5");

    // object macros

    assert_pp("#define X 42\nX", "42");

    assert_pp("#define NAME hello\nNAME", "hello");

    assert_pp("#define EMPTY\nEMPTY", "");

    // mehrfach expansion

    assert_pp("#define A B\n#define B C\n#define C 123\nA", "123");

    // undef
    assert_pp("#define X 5\n#undef X\nX", "X");

    // whitespace
    assert_pp("#define X       10\nX", "10");

    assert_pp("#define X 10\n\n\nX", "10");

    // kommentare
    assert_pp("#define X 5\n/* hello */ X", "  5");

    assert_pp("// comment\n#define X 5\nX", "5");

    assert_pp("int/**/x;", "int x;");

    // ifdef
    assert_pp("#define DEBUG\n#ifdef DEBUG\nYES\n#endif", "YES");

    assert_pp("#ifdef DEBUG\nYES\n#endif", "");

    // ifndef
    assert_pp("#ifndef DEBUG\nYES\n#endif", "YES");

    // else
    assert_pp("#ifdef DEBUG\nYES\n#else\nNO\n#endif", "NO");

    // Verschachtelte Bedingungen
    assert_pp("#define A\n#ifdef A\n#ifndef "
              "B\nOK\n#endif\n#endif",
              "OK");;

    // Function macros
    assert_pp("#define ADD(a,b) a+b\nADD(2,3)", "2+3");

    // mehrere parameter
    assert_pp("#define MUL(a,b,c) a*b*c\nMUL(2,3,4)", "2*3*4");

    // leerzeichen
    assert_pp("#define F(x) x\nF( hello )", "hello");

    // verschachtelte macros
    assert_pp("#define X(a) a\n#define Y 10\nX(Y)", "10");

    // makro als argument
    assert_pp("#define VAL 5\n#define DOUBLE(x) "
              "x+x\nDOUBLE(VAL)",
              "5+5");

    // expansion edge cases
    assert_pp("#define X X\nX", "X");

    // zyklus
    assert_pp("#define A B\n#define B A\nA", "A");

    // ketten
    assert_pp("#define A B\n#define B C\n#define C "
              "D\n#define D E\n#define E 1\nA",
              "1");

    // if expression test
    assert_pp("#if 1\nYES\n#endif", "YES");

    assert_pp("#if 0\nYES\n#endif", "");

    // vergleich
    assert_pp("#if 10 > 5\nYES\n#endif", "YES");

    assert_pp("#if 10 < 5\nYES\n#endif", "");

    assert_pp("#if 5 == 5\nYES\n#endif", "YES");

    // operator
    assert_pp("#if 1 && 1\nYES\n#endif", "YES");

    assert_pp("#if 1 || 0\nYES\n#endif", "YES");

    assert_pp("#if !0\nYES\n#endif", "YES");

    // defined
    assert_pp("#define X\n#if defined(X)\nYES\n#endif", "YES");

    assert_pp("#if !defined(X)\nYES\n#endif", "YES");

    // stringification
    assert_pp("#if !defined(X)\nYES\n#endif", "YES");

    // whitespace normalize
    assert_pp("#define STR(x) #x\nSTR(hello   world)", "\"hello world\"");

    // string escape
    assert_pp("#define STR(x) #x\nSTR(\"abc\")", "\"\\\"abc\\\"\"");

    // token pasting
    assert_pp("#define CAT(a,b) a##b\nCAT(foo,bar)", "foobar");

    // zahlen
    assert_pp("#define CAT(a,b) a##b\nCAT(12,34)", "1234");

    // mit markros
    assert_pp("#define A hello\n#define CAT(a,b) "
              "a##b\nCAT(A,x)",
              "Ax");

    // variadic macros
    assert_pp("#define LOG(...) __VA_ARGS__\nLOG(a,b,c)", "a,b,c");

    // normal parameter
    assert_pp("#define PRINT(x,...) x "
              "__VA_ARGS__\nPRINT(a,b,c)",
              "a b,c");

    //// include test
    // assert_pp_file("main.c", "#include \"a.h\"",
    //                {{"a.h", "#define X 5"}}, "5");

    //// include guard
    // assert_pp_file(
    //     "main.c",
    //     "#include \"a.h\"\n#include \"a.h\"",
    //     {{"a.h", "#ifndef A_H\n#define "
    //              "A_H\nHELLO\n#endif"}},
    //     "HELLO");

    // expansion timing
    assert_pp("#define A B\n#define B 1\n#define "
              "F(x) x\nF(A)",
              "1");

    // argument not expand
    assert_pp("#define A B\n#define B 1\n#define "
              "F(x) x\nF(A)",
              "1");

    assert_pp("#define CAT(a,b) a##b\n#define A "
              "10\nCAT(A,x)",
              "Ax");

    assert_pp("#define X foo\n#define CAT(a,b) "
              "a##b\n#define foo 123\nCAT(f,oo)",
              "123");

    // empty arguments
    assert_pp("#define CAT(a,b) a##b\nCAT(,foo)", "foo");

    return 0;
}

#include "test.hpp"

using namespace fling;
using namespace fling::ast;
using namespace fling::lexer;
using namespace fling::parser;

TEST(test_identifier_parse) {
    Parser p({Token{Identifier, "abc"}});
    auto *expr = p.parse_primary_expr();
    EXPECT(expr != nullptr);
}

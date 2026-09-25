#include <gtest/gtest.h>
#include "lexer/lexer.h"
#include <stdexcept>

TEST(LexerTest, KeywordsCaseInsensitive) {
    std::string sql = "CREATE TABLE DROP DESCRIBE SHOW TABLES INSERT INTO VALUES "
                      "UPDATE SET DELETE SELECT FROM WHERE ORDER BY LIMIT ASC DESC "
                      "AND PRIMARY KEY INT FLOAT STRING BOOL SEMANTIC AS OF "
                      "BETWEEN SNAPSHOT COMPARE EVOLUTION HISTORY ROLLBACK TO COMPACT SIMILAR";
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<TokenType> expected = {
        TokenType::CREATE, TokenType::TABLE, TokenType::DROP, TokenType::DESCRIBE,
        TokenType::SHOW, TokenType::TABLES, TokenType::INSERT, TokenType::INTO, TokenType::VALUES,
        TokenType::UPDATE, TokenType::SET, TokenType::DELETE, TokenType::SELECT, TokenType::FROM,
        TokenType::WHERE, TokenType::ORDER, TokenType::BY, TokenType::LIMIT, TokenType::ASC, TokenType::DESC,
        TokenType::AND, TokenType::PRIMARY, TokenType::KEY, TokenType::INT, TokenType::FLOAT,
        TokenType::STRING, TokenType::BOOL, TokenType::SEMANTIC, TokenType::AS, TokenType::OF,
        TokenType::BETWEEN, TokenType::SNAPSHOT, TokenType::COMPARE, TokenType::EVOLUTION, TokenType::HISTORY,
        TokenType::ROLLBACK, TokenType::TO, TokenType::COMPACT, TokenType::SIMILAR,
        TokenType::END_OF_FILE
    };

    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }

    Lexer lowerLexer("create table select from where");
    std::vector<Token> lowerTokens = lowerLexer.tokenize();
    EXPECT_EQ(lowerTokens[0].type, TokenType::CREATE);
    EXPECT_EQ(lowerTokens[1].type, TokenType::TABLE);
    EXPECT_EQ(lowerTokens[2].type, TokenType::SELECT);
    EXPECT_EQ(lowerTokens[3].type, TokenType::FROM);
    EXPECT_EQ(lowerTokens[4].type, TokenType::WHERE);
}

TEST(LexerTest, Identifiers) {
    Lexer lexer("users user_id _private col123");
    std::vector<Token> tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].value, "users");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].value, "user_id");
    EXPECT_EQ(tokens[2].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].value, "_private");
    EXPECT_EQ(tokens[3].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[3].value, "col123");
    EXPECT_EQ(tokens[4].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, Literals) {
    Lexer lexer("123 0 45.67 'hello world' \"escaped\\nstring\"");
    std::vector<Token> tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[0].value, "123");

    EXPECT_EQ(tokens[1].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[1].value, "0");

    EXPECT_EQ(tokens[2].type, TokenType::FLOAT_LITERAL);
    EXPECT_EQ(tokens[2].value, "45.67");

    EXPECT_EQ(tokens[3].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[3].value, "hello world");

    EXPECT_EQ(tokens[4].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[4].value, "escaped\nstring");

    EXPECT_EQ(tokens[5].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, UnterminatedStringThrows) {
    Lexer lexer("'unclosed string");
    EXPECT_THROW(lexer.tokenize(), std::runtime_error);
}

TEST(LexerTest, OperatorsAndSymbols) {
    Lexer lexer("= != < <= > >= * ( ) , ; - :");
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<TokenType> expected = {
        TokenType::EQUAL, TokenType::NOT_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL,
        TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::STAR, TokenType::LPAREN,
        TokenType::RPAREN, TokenType::COMMA, TokenType::SEMICOLON, TokenType::MINUS,
        TokenType::COLON, TokenType::END_OF_FILE
    };

    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }
}

TEST(LexerTest, LineAndColumnTracking) {
    std::string sql = "SELECT\n  id,\n  name";
    Lexer lexer(sql);

    Token t1 = lexer.nextToken();
    EXPECT_EQ(t1.type, TokenType::SELECT);
    EXPECT_EQ(t1.line, 1);
    EXPECT_EQ(t1.column, 1);

    Token t2 = lexer.nextToken();
    EXPECT_EQ(t2.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t2.value, "id");
    EXPECT_EQ(t2.line, 2);
    EXPECT_EQ(t2.column, 3);

    Token t3 = lexer.nextToken();
    EXPECT_EQ(t3.type, TokenType::COMMA);
    EXPECT_EQ(t3.line, 2);
    EXPECT_EQ(t3.column, 5);

    Token t4 = lexer.nextToken();
    EXPECT_EQ(t4.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t4.value, "name");
    EXPECT_EQ(t4.line, 3);
    EXPECT_EQ(t4.column, 3);
}

TEST(LexerTest, UnknownToken) {
    Lexer lexer("@ # $");
    Token t1 = lexer.nextToken();
    EXPECT_EQ(t1.type, TokenType::UNKNOWN);
    EXPECT_EQ(t1.value, "@");

    Token t2 = lexer.nextToken();
    EXPECT_EQ(t2.type, TokenType::UNKNOWN);
    EXPECT_EQ(t2.value, "#");

    Token t3 = lexer.nextToken();
    EXPECT_EQ(t3.type, TokenType::UNKNOWN);
    EXPECT_EQ(t3.value, "$");
}

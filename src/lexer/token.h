#pragma once

#include <string>

enum class TokenType {
    END_OF_FILE,
    UNKNOWN,

    CREATE,
    DROP,
    TABLE,

    INSERT,
    INTO,
    VALUES,

    UPDATE,
    SET,

    DELETE,

    SELECT,
    FROM,

    WHERE,
    ORDER,
    BY,
    LIMIT,
    ASC,
    DESC,
    AND,
    MINUS,

    PRIMARY,
    KEY,

    INT,
    FLOAT,
    STRING,
    BOOL,

    SEMANTIC,

    AS,
    OF,
    BETWEEN,
    SNAPSHOT,

    COMPARE,
    EVOLUTION,
    HISTORY,
    COMPACT,

    ROLLBACK,
    TO,

    SIMILAR,

    IDENTIFIER,

    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,

    EQUAL,          
    NOT_EQUAL,      
    LESS,           
    LESS_EQUAL,     
    GREATER,        
    GREATER_EQUAL,  

    STAR,           
    LPAREN,         
    RPAREN,         
    COMMA,          
    SEMICOLON       
};

struct Token {

    TokenType type;
    std::string value;
    int line;
    int column;
};

std::string tokenTypeToString(TokenType type);
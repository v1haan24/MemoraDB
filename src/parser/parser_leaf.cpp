#include "parser.h"
Value Parser::parseValue() {
    if (check(TokenType::INTEGER_LITERAL)) {
        const Token& tok = advance();
        Value v;
        v.kind = Value::Kind::INT;
        v.raw = tok.value;
        v.intVal = std::stoll(tok.value);
        return v;
    }
    if (check(TokenType::FLOAT_LITERAL)) {
        const Token& tok = advance();
        Value v;
        v.kind = Value::Kind::FLOAT;
        v.raw = tok.value;
        v.floatVal = std::stod(tok.value);
        return v;
    }
    if (check(TokenType::STRING_LITERAL)) {
        const Token& tok = advance();
        Value v;
        v.kind = Value::Kind::STRING;
        v.raw = tok.value;
        v.strVal = tok.value;
        return v;
    }

    const Token& actual = peek();
    throw ParseError(
        "Expected a value (integer, float, or string literal) but got " +
            tokenTypeToString(actual.type) +
            " at line " + std::to_string(actual.line) + ", column " + std::to_string(actual.column),
        actual.line, actual.column);
}
DateLiteral Parser::parseDateLiteral() {
    const Token& yearTok = expect(TokenType::INTEGER_LITERAL, "for the year in a date literal (expected YYYY-MM-DD)");
    expect(TokenType::MINUS, "in date literal (expected YYYY-MM-DD)");
    const Token& monthTok = expect(TokenType::INTEGER_LITERAL, "for the month in a date literal (expected YYYY-MM-DD)");
    expect(TokenType::MINUS, "in date literal (expected YYYY-MM-DD)");
    const Token& dayTok = expect(TokenType::INTEGER_LITERAL, "for the day in a date literal (expected YYYY-MM-DD)");

    DateLiteral date;
    date.year = std::stoi(yearTok.value);
    date.month = std::stoi(monthTok.value);
    date.day = std::stoi(dayTok.value);
    if (date.month < 1 || date.month > 12) {
        throw ParseError("Invalid month " + std::to_string(date.month) +
                              " in date literal (must be between 1 and 12)",
                          monthTok.line, monthTok.column);
    }
    if (date.day < 1 || date.day > 31) {
        throw ParseError("Invalid day " + std::to_string(date.day) +
                              " in date literal (must be between 1 and 31)",
                          dayTok.line, dayTok.column);
    }

    return date;
}
CompareOp Parser::parseCompareOp() {
    if (match(TokenType::EQUAL)) return CompareOp::EQ;
    if (match(TokenType::NOT_EQUAL)) return CompareOp::NE;
    if (match(TokenType::LESS_EQUAL)) return CompareOp::LE;
    if (match(TokenType::LESS)) return CompareOp::LT;
    if (match(TokenType::GREATER_EQUAL)) return CompareOp::GE;
    if (match(TokenType::GREATER)) return CompareOp::GT;

    const Token& actual = peek();
    throw ParseError(
        "Expected a comparison operator (=, !=, <, <=, >, >=) but got " +
            tokenTypeToString(actual.type),
        actual.line, actual.column);
}
Condition Parser::parseCondition() {
    const Token& colTok = expect(TokenType::IDENTIFIER, "as the column name in a condition");
    Condition cond;
    cond.column = colTok.value;
    if (check(TokenType::SIMILAR)) {
        advance(); 
        expect(TokenType::TO, "after SIMILAR (expected 'SIMILAR TO \"text\"')");
        const Token& textTok = expect(TokenType::STRING_LITERAL, "after SIMILAR TO");
        cond.op = CompareOp::SIMILAR_TO;
        Value v;
        v.kind = Value::Kind::STRING;
        v.raw = textTok.value;
        v.strVal = textTok.value;
        cond.value = v;
        return cond;
    }
    cond.op = parseCompareOp();
    cond.value = parseValue();
    return cond;
}
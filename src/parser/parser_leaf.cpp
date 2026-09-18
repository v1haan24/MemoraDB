#include "parser.h"

static bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int daysInMonth(int year, int month) {
    static const int lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 31; 
    if (month == 2 && isLeapYear(year)) return 29;
    return lengths[month - 1];
}
Value Parser::parseValue() {
    bool negative = false;
    Token minusTok;
    if (check(TokenType::MINUS)) {
        minusTok = advance();
        negative = true;
    }
    if (check(TokenType::INTEGER_LITERAL)) {
        const Token& tok = advance();
        Value v;
        v.kind = Value::Kind::INT;
        v.raw = negative ? "-" + tok.value : tok.value;
        v.intVal = std::stoll(v.raw);
        return v;
    }
    if (check(TokenType::FLOAT_LITERAL)) {
        const Token& tok = advance();
        Value v;
        v.kind = Value::Kind::FLOAT;
        v.raw = negative ? "-" + tok.value : tok.value;
        v.floatVal = std::stod(v.raw);
        return v;
    }
    if (!negative && check(TokenType::STRING_LITERAL)) {
        const Token& tok = advance();
        Value v;
        v.kind = Value::Kind::STRING;
        v.raw = tok.value;
        v.strVal = tok.value;
        return v;
    }
    if (negative) {
        const Token& actual = peek();
        throw ParseError(
            "Expected an integer or float literal after unary '-' but got " +
                tokenTypeToString(actual.type) +
                " at line " + std::to_string(actual.line) + ", column " + std::to_string(actual.column),
            minusTok.line, minusTok.column);
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
    int maxDay = daysInMonth(date.year, date.month);
    if (date.day < 1 || date.day > maxDay) {
        throw ParseError("Invalid day " + std::to_string(date.day) +
                              " in date literal (month " + std::to_string(date.month) +
                              " of year " + std::to_string(date.year) +
                              " has " + std::to_string(maxDay) + " days)",
                          dayTok.line, dayTok.column);
    }
    if (check(TokenType::INTEGER_LITERAL)) {
        const Token& hourTok = advance();
        expect(TokenType::COLON, "between hour and minute in a timestamp (expected HH:MM[:SS])");
        const Token& minuteTok = expect(TokenType::INTEGER_LITERAL,
                                        "for the minute in a timestamp (expected HH:MM[:SS])");

        date.hour = std::stoi(hourTok.value);
        date.minute = std::stoi(minuteTok.value);
        date.second = 0;
        date.hasTime = true;

        if (match(TokenType::COLON)) {
            const Token& secondTok = expect(TokenType::INTEGER_LITERAL,
                                            "for the second in a timestamp (expected HH:MM:SS)");
            date.second = std::stoi(secondTok.value);
        }

        if (date.hour < 0 || date.hour > 23) {
            throw ParseError("Invalid hour " + std::to_string(date.hour) +
                                  " in timestamp (must be between 0 and 23)",
                              hourTok.line, hourTok.column);
        }
        if (date.minute < 0 || date.minute > 59) {
            throw ParseError("Invalid minute " + std::to_string(date.minute) +
                                  " in timestamp (must be between 0 and 59)",
                              minuteTok.line, minuteTok.column);
        }
        if (date.second < 0 || date.second > 59) {
            throw ParseError("Invalid second " + std::to_string(date.second) +
                                  " in timestamp (must be between 0 and 59)",
                              hourTok.line, hourTok.column);
        }
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
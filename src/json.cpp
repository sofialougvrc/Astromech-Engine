#include "ace/json.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

namespace ace {

Json::Json() : value_(nullptr) {}
Json::Json(Value value) : value_(std::move(value)) {}

bool Json::isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Json::isBool() const { return std::holds_alternative<bool>(value_); }
bool Json::isNumber() const { return std::holds_alternative<double>(value_); }
bool Json::isString() const { return std::holds_alternative<std::string>(value_); }
bool Json::isArray() const { return std::holds_alternative<Array>(value_); }
bool Json::isObject() const { return std::holds_alternative<Object>(value_); }

bool Json::asBool() const { return std::get<bool>(value_); }
double Json::asNumber() const { return std::get<double>(value_); }
int Json::asInt() const { return static_cast<int>(std::get<double>(value_)); }
const std::string& Json::asString() const { return std::get<std::string>(value_); }
const Json::Array& Json::asArray() const { return std::get<Array>(value_); }
const Json::Object& Json::asObject() const { return std::get<Object>(value_); }

bool Json::contains(const std::string& key) const {
    return isObject() && asObject().contains(key);
}

const Json& Json::at(const std::string& key) const {
    const auto& object = asObject();
    auto found = object.find(key);
    if (found == object.end()) {
        throw JsonError("missing key: " + key);
    }
    return found->second;
}

std::string Json::stringOr(const std::string& key, const std::string& fallback) const {
    if (!contains(key)) {
        return fallback;
    }
    return at(key).asString();
}

int Json::intOr(const std::string& key, int fallback) const {
    if (!contains(key)) {
        return fallback;
    }
    return at(key).asInt();
}

class Parser {
public:
    explicit Parser(std::string text) : text_(std::move(text)) {}

    Json parse() {
        skipWhitespace();
        Json value = parseValue();
        skipWhitespace();
        if (pos_ != text_.size()) {
            fail("unexpected trailing content");
        }
        return value;
    }

private:
    Json parseValue() {
        skipWhitespace();
        if (pos_ >= text_.size()) {
            fail("unexpected end of input");
        }

        const char c = text_[pos_];
        if (c == '{') {
            return Json(parseObject());
        }
        if (c == '[') {
            return Json(parseArray());
        }
        if (c == '"') {
            return Json(parseString());
        }
        if (c == 't') {
            consumeLiteral("true");
            return Json(true);
        }
        if (c == 'f') {
            consumeLiteral("false");
            return Json(false);
        }
        if (c == 'n') {
            consumeLiteral("null");
            return Json(nullptr);
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return Json(parseNumber());
        }
        fail("unexpected token");
    }

    Json::Object parseObject() {
        expect('{');
        Json::Object object;
        skipWhitespace();
        if (peek('}')) {
            ++pos_;
            return object;
        }
        while (true) {
            skipWhitespace();
            if (!peek('"')) {
                fail("object keys must be strings");
            }
            std::string key = parseString();
            skipWhitespace();
            expect(':');
            object.emplace(std::move(key), parseValue());
            skipWhitespace();
            if (peek('}')) {
                ++pos_;
                break;
            }
            expect(',');
        }
        return object;
    }

    Json::Array parseArray() {
        expect('[');
        Json::Array array;
        skipWhitespace();
        if (peek(']')) {
            ++pos_;
            return array;
        }
        while (true) {
            array.push_back(parseValue());
            skipWhitespace();
            if (peek(']')) {
                ++pos_;
                break;
            }
            expect(',');
        }
        return array;
    }

    std::string parseString() {
        expect('"');
        std::string result;
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') {
                return result;
            }
            if (c == '\\') {
                if (pos_ >= text_.size()) {
                    fail("unterminated escape sequence");
                }
                const char escaped = text_[pos_++];
                switch (escaped) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    default: fail("unsupported escape sequence");
                }
            } else {
                result.push_back(c);
            }
        }
        fail("unterminated string");
    }

    double parseNumber() {
        const auto start = pos_;
        if (peek('-')) {
            ++pos_;
        }
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
        if (peek('.')) {
            ++pos_;
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
            }
        }
        return std::stod(text_.substr(start, pos_ - start));
    }

    void consumeLiteral(const std::string& literal) {
        if (text_.substr(pos_, literal.size()) != literal) {
            fail("expected " + literal);
        }
        pos_ += literal.size();
    }

    void skipWhitespace() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool peek(char c) const {
        return pos_ < text_.size() && text_[pos_] == c;
    }

    void expect(char c) {
        skipWhitespace();
        if (!peek(c)) {
            fail(std::string("expected '") + c + "'");
        }
        ++pos_;
    }

    [[noreturn]] void fail(const std::string& message) const {
        throw JsonError(message + " at byte " + std::to_string(pos_));
    }

    std::string text_;
    std::size_t pos_ = 0;
};

Json parseJson(const std::string& text) {
    return Parser(text).parse();
}

Json parseJsonFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw JsonError("unable to open JSON file: " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parseJson(buffer.str());
}

std::string jsonScalarToString(const Json& value) {
    if (value.isString()) {
        return value.asString();
    }
    if (value.isNumber()) {
        const double number = value.asNumber();
        const int integer = static_cast<int>(number);
        if (number == integer) {
            return std::to_string(integer);
        }
        return std::to_string(number);
    }
    if (value.isBool()) {
        return value.asBool() ? "true" : "false";
    }
    if (value.isNull()) {
        return "null";
    }
    throw JsonError("expected scalar JSON value");
}

} // namespace ace

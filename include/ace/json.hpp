#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace ace {

class JsonError : public std::runtime_error {
public:
    explicit JsonError(const std::string& message) : std::runtime_error(message) {}
};

class Json {
public:
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;
    using Value = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    Json();
    explicit Json(Value value);

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool asBool() const;
    double asNumber() const;
    int asInt() const;
    const std::string& asString() const;
    const Array& asArray() const;
    const Object& asObject() const;

    bool contains(const std::string& key) const;
    const Json& at(const std::string& key) const;
    std::string stringOr(const std::string& key, const std::string& fallback) const;
    int intOr(const std::string& key, int fallback) const;

private:
    Value value_;
};

Json parseJson(const std::string& text);
Json parseJsonFile(const std::string& path);
std::string jsonScalarToString(const Json& value);

} // namespace ace

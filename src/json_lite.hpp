// =============================================================================
// BudgetBite - Affordable Meal Planning System
// File: src/json_lite.hpp
// Description: Minimal, self-contained JSON value parser (no external libs).
//
// Only the subset of JSON needed by this project is implemented:
//   - Objects  { "key": value, ... }
//   - Arrays   [ value, ... ]
//   - Strings  "..."   (with \" and \\ escapes)
//   - Numbers  (integer and floating-point)
//   - null, true, false
//
// Usage:
//   JsonValue v = json_parse(json_string);
//   v["key"].as_string()
//   v[0]["ingredient_name"].as_string()
// =============================================================================
#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cctype>

struct JsonValue;
using JsonObject = std::map<std::string, JsonValue>;
using JsonArray  = std::vector<JsonValue>;

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;

    bool        b_val  = false;
    double      n_val  = 0.0;
    std::string s_val;
    JsonArray   a_val;
    JsonObject  o_val;

    // --- Accessors ---
    bool        is_null()   const { return type == Type::Null; }
    bool        is_string() const { return type == Type::String; }
    bool        is_number() const { return type == Type::Number; }
    bool        is_array()  const { return type == Type::Array; }
    bool        is_object() const { return type == Type::Object; }

    const std::string& as_string()  const { return s_val; }
    double             as_double()  const { return n_val; }
    bool               as_bool()    const { return b_val; }
    const JsonArray&   as_array()   const { return a_val; }
    const JsonObject&  as_object()  const { return o_val; }

    // Subscript by key (object)
    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null_val;
        auto it = o_val.find(key);
        return (it != o_val.end()) ? it->second : null_val;
    }
    // Subscript by index (array)
    const JsonValue& operator[](size_t idx) const {
        static JsonValue null_val;
        return (idx < a_val.size()) ? a_val[idx] : null_val;
    }

    // Safe string retrieval (returns "" for non-string)
    std::string str(const std::string& def = "") const {
        return is_string() ? s_val : def;
    }
    double num(double def = 0.0) const {
        return is_number() ? n_val : def;
    }
};

// ── Parser internals ──────────────────────────────────────────────────────────
namespace json_detail {

struct Parser {
    const char* p;
    const char* end;

    explicit Parser(const std::string& s) : p(s.data()), end(s.data() + s.size()) {}

    void skip_ws() {
        while (p < end && std::isspace(static_cast<unsigned char>(*p))) ++p;
    }

    char peek() const { return (p < end) ? *p : '\0'; }
    char consume() { return (p < end) ? *p++ : '\0'; }

    bool expect(char c) { if (peek() == c) { ++p; return true; } return false; }

    std::string parse_string() {
        if (consume() != '"') throw std::runtime_error("Expected '\"'");
        std::string out;
        while (p < end) {
            char c = consume();
            if (c == '"') return out;
            if (c == '\\') {
                char e = consume();
                switch(e) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    default:   out += e;    break;
                }
            } else {
                out += c;
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    double parse_number() {
        const char* start = p;
        if (peek() == '-') ++p;
        while (p < end && std::isdigit(static_cast<unsigned char>(*p))) ++p;
        if (p < end && *p == '.') {
            ++p;
            while (p < end && std::isdigit(static_cast<unsigned char>(*p))) ++p;
        }
        if (p < end && (*p == 'e' || *p == 'E')) {
            ++p;
            if (p < end && (*p == '+' || *p == '-')) ++p;
            while (p < end && std::isdigit(static_cast<unsigned char>(*p))) ++p;
        }
        std::string s(start, p);
        try { return std::stod(s); } catch (...) { return 0.0; }
    }

    JsonValue parse_value() {
        skip_ws();
        JsonValue v;
        char c = peek();
        if (c == '"') {
            v.type  = JsonValue::Type::String;
            v.s_val = parse_string();
        } else if (c == '{') {
            v.type = JsonValue::Type::Object;
            ++p;
            skip_ws();
            if (peek() != '}') {
                while (true) {
                    skip_ws();
                    std::string key = parse_string();
                    skip_ws();
                    if (!expect(':')) throw std::runtime_error("Expected ':'");
                    v.o_val[key] = parse_value();
                    skip_ws();
                    if (peek() == '}') break;
                    if (!expect(',')) throw std::runtime_error("Expected ',' or '}'");
                }
            }
            expect('}');
        } else if (c == '[') {
            v.type = JsonValue::Type::Array;
            ++p;
            skip_ws();
            if (peek() != ']') {
                while (true) {
                    v.a_val.push_back(parse_value());
                    skip_ws();
                    if (peek() == ']') break;
                    if (!expect(',')) throw std::runtime_error("Expected ',' or ']'");
                }
            }
            expect(']');
        } else if (c == 't') {
            p += 4; // "true"
            v.type = JsonValue::Type::Bool; v.b_val = true;
        } else if (c == 'f') {
            p += 5; // "false"
            v.type = JsonValue::Type::Bool; v.b_val = false;
        } else if (c == 'n') {
            p += 4; // "null"
            v.type = JsonValue::Type::Null;
        } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            v.type  = JsonValue::Type::Number;
            v.n_val = parse_number();
        } else {
            throw std::runtime_error(std::string("Unexpected character: ") + c);
        }
        return v;
    }
};

} // namespace json_detail

// ── Public entry point ────────────────────────────────────────────────────────
inline JsonValue json_parse(const std::string& s) {
    json_detail::Parser parser(s);
    return parser.parse_value();
}

// Safe wrapper that returns null-type on parse failure
inline JsonValue json_try_parse(const std::string& s) {
    try { return json_parse(s); } catch (...) { return JsonValue{}; }
}

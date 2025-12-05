// cppx.serialization - Macro definitions
// Simplified macros for MSVC + C++20 modules compatibility

#pragma once

// ============================================================================
// Helper macros for individual field serialization
// ============================================================================

// Serialize a single field to JSON
#define CPPX_SER_FIELD(obj, field) \
    j[#field] = cppx::serializer<std::remove_cvref_t<decltype(obj.field)>>::to_json(obj.field);

// Deserialize a single field from JSON
#define CPPX_DESER_FIELD(obj, json_obj, field) \
    if (json_obj.contains(#field)) { \
        obj.field = cppx::serializer<std::remove_cvref_t<decltype(obj.field)>>::from_json(json_obj[#field]); \
    }

// ============================================================================
// Serialization macros for different field counts (1-10 fields)
// ============================================================================

// 1 field
#define CPPX_SERIALIZABLE_1(Type, f1) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1) \
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1) \
            return value; \
        } \
    }; \
    }

// 2 fields
#define CPPX_SERIALIZABLE_2(Type, f1, f2) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1) \
            CPPX_SER_FIELD(value, f2) \
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1) \
            CPPX_DESER_FIELD(value, j_param, f2) \
            return value; \
        } \
    }; \
    }

// 3 fields
#define CPPX_SERIALIZABLE_3(Type, f1, f2, f3) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1) \
            CPPX_SER_FIELD(value, f2) \
            CPPX_SER_FIELD(value, f3) \
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1) \
            CPPX_DESER_FIELD(value, j_param, f2) \
            CPPX_DESER_FIELD(value, j_param, f3) \
            return value; \
        } \
    }; \
    }

// 4 fields
#define CPPX_SERIALIZABLE_4(Type, f1, f2, f3, f4) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1) \
            CPPX_SER_FIELD(value, f2) \
            CPPX_SER_FIELD(value, f3) \
            CPPX_SER_FIELD(value, f4) \
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1) \
            CPPX_DESER_FIELD(value, j_param, f2) \
            CPPX_DESER_FIELD(value, j_param, f3) \
            CPPX_DESER_FIELD(value, j_param, f4) \
            return value; \
        } \
    }; \
    }

// 5 fields
#define CPPX_SERIALIZABLE_5(Type, f1, f2, f3, f4, f5) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1) \
            CPPX_SER_FIELD(value, f2) \
            CPPX_SER_FIELD(value, f3) \
            CPPX_SER_FIELD(value, f4) \
            CPPX_SER_FIELD(value, f5) \
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1) \
            CPPX_DESER_FIELD(value, j_param, f2) \
            CPPX_DESER_FIELD(value, j_param, f3) \
            CPPX_DESER_FIELD(value, j_param, f4) \
            CPPX_DESER_FIELD(value, j_param, f5) \
            return value; \
        } \
    }; \
    }

// 6 fields
#define CPPX_SERIALIZABLE_6(Type, f1, f2, f3, f4, f5, f6) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1) \
            CPPX_SER_FIELD(value, f2) \
            CPPX_SER_FIELD(value, f3) \
            CPPX_SER_FIELD(value, f4) \
            CPPX_SER_FIELD(value, f5) \
            CPPX_SER_FIELD(value, f6) \
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1) \
            CPPX_DESER_FIELD(value, j_param, f2) \
            CPPX_DESER_FIELD(value, j_param, f3) \
            CPPX_DESER_FIELD(value, j_param, f4) \
            CPPX_DESER_FIELD(value, j_param, f5) \
            CPPX_DESER_FIELD(value, j_param, f6) \
            return value; \
        } \
    }; \
    }

// 7 fields
#define CPPX_SERIALIZABLE_7(Type, f1, f2, f3, f4, f5, f6, f7) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1) \
            CPPX_SER_FIELD(value, f2) \
            CPPX_SER_FIELD(value, f3) \
            CPPX_SER_FIELD(value, f4) \
            CPPX_SER_FIELD(value, f5) \
            CPPX_SER_FIELD(value, f6) \
            CPPX_SER_FIELD(value, f7) \
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1) \
            CPPX_DESER_FIELD(value, j_param, f2) \
            CPPX_DESER_FIELD(value, j_param, f3) \
            CPPX_DESER_FIELD(value, j_param, f4) \
            CPPX_DESER_FIELD(value, j_param, f5) \
            CPPX_DESER_FIELD(value, j_param, f6) \
            CPPX_DESER_FIELD(value, j_param, f7) \
            return value; \
        } \
    }; \
    }

// 8 fields
#define CPPX_SERIALIZABLE_8(Type, f1, f2, f3, f4, f5, f6, f7, f8) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1) \
            CPPX_SER_FIELD(value, f2) \
            CPPX_SER_FIELD(value, f3) \
            CPPX_SER_FIELD(value, f4) \
            CPPX_SER_FIELD(value, f5) \
            CPPX_SER_FIELD(value, f6) \
            CPPX_SER_FIELD(value, f7) \
            CPPX_SER_FIELD(value, f8) \
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1) \
            CPPX_DESER_FIELD(value, j_param, f2) \
            CPPX_DESER_FIELD(value, j_param, f3) \
            CPPX_DESER_FIELD(value, j_param, f4) \
            CPPX_DESER_FIELD(value, j_param, f5) \
            CPPX_DESER_FIELD(value, j_param, f6) \
            CPPX_DESER_FIELD(value, j_param, f7) \
            CPPX_DESER_FIELD(value, j_param, f8) \
            return value; \
        } \
    }; \
    }

// ============================================================================
// Macro overloading based on argument count
// ============================================================================

// Count arguments: Type + fields, so 2 args = 1 field, 3 args = 2 fields, etc.
#define CPPX_GET_MACRO2(_1, _2, NAME, ...) NAME
#define CPPX_GET_MACRO3(_1, _2, _3, NAME, ...) NAME
#define CPPX_GET_MACRO4(_1, _2, _3, _4, NAME, ...) NAME
#define CPPX_GET_MACRO5(_1, _2, _3, _4, _5, NAME, ...) NAME
#define CPPX_GET_MACRO6(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define CPPX_GET_MACRO7(_1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define CPPX_GET_MACRO8(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define CPPX_GET_MACRO9(_1, _2, _3, _4, _5, _6, _7, _8, _9, NAME, ...) NAME

/**
 * CPPX_SERIALIZABLE - Register a type for serialization (supports 1-8 fields)
 * 
 * Usage:
 *   struct Point { int x, y; };
 *   CPPX_SERIALIZABLE(Point, x, y)
 * 
 *   struct Person { 
 *       std::string name;
 *       int age;
 *       std::vector<std::string> hobbies;
 *   };
 *   CPPX_SERIALIZABLE(Person, name, age, hobbies)
 */
#define CPPX_SERIALIZABLE(...) \
    CPPX_GET_MACRO9(__VA_ARGS__, \
        CPPX_SERIALIZABLE_8, \
        CPPX_SERIALIZABLE_7, \
        CPPX_SERIALIZABLE_6, \
        CPPX_SERIALIZABLE_5, \
        CPPX_SERIALIZABLE_4, \
        CPPX_SERIALIZABLE_3, \
        CPPX_SERIALIZABLE_2, \
        CPPX_SERIALIZABLE_1, \
        invalid \
    )(__VA_ARGS__)

// ============================================================================
// Enum serialization macros
// ============================================================================

// Helper macro for enum to string case
#define CPPX_ENUM_CASE(EnumType, value) \
    case EnumType::value: return #value;

// 1 value
#define CPPX_ENUM_SERIALIZABLE_1(EnumType, v1) \
    namespace cppx { \
    inline std::string enum_to_string_##EnumType(EnumType value) { \
        switch (value) { \
            CPPX_ENUM_CASE(EnumType, v1) \
            default: return "Unknown"; \
        } \
    } \
    inline EnumType enum_from_string_##EnumType(const std::string& str) { \
        if (str == #v1) return EnumType::v1; \
        throw std::runtime_error("invalid enum string: " + str); \
    } \
    template<> \
    struct serializer<EnumType> { \
        static json to_json(const EnumType& value) { \
            return json(enum_to_string_##EnumType(value)); \
        } \
        static EnumType from_json(const json& j) { \
            return enum_from_string_##EnumType(j.as_string()); \
        } \
    }; \
    }

// 2 values
#define CPPX_ENUM_SERIALIZABLE_2(EnumType, v1, v2) \
    namespace cppx { \
    inline std::string enum_to_string_##EnumType(EnumType value) { \
        switch (value) { \
            CPPX_ENUM_CASE(EnumType, v1) \
            CPPX_ENUM_CASE(EnumType, v2) \
            default: return "Unknown"; \
        } \
    } \
    inline EnumType enum_from_string_##EnumType(const std::string& str) { \
        if (str == #v1) return EnumType::v1; \
        if (str == #v2) return EnumType::v2; \
        throw std::runtime_error("invalid enum string: " + str); \
    } \
    template<> \
    struct serializer<EnumType> { \
        static json to_json(const EnumType& value) { \
            return json(enum_to_string_##EnumType(value)); \
        } \
        static EnumType from_json(const json& j) { \
            return enum_from_string_##EnumType(j.as_string()); \
        } \
    }; \
    }

// 3 values
#define CPPX_ENUM_SERIALIZABLE_3(EnumType, v1, v2, v3) \
    namespace cppx { \
    inline std::string enum_to_string_##EnumType(EnumType value) { \
        switch (value) { \
            CPPX_ENUM_CASE(EnumType, v1) \
            CPPX_ENUM_CASE(EnumType, v2) \
            CPPX_ENUM_CASE(EnumType, v3) \
            default: return "Unknown"; \
        } \
    } \
    inline EnumType enum_from_string_##EnumType(const std::string& str) { \
        if (str == #v1) return EnumType::v1; \
        if (str == #v2) return EnumType::v2; \
        if (str == #v3) return EnumType::v3; \
        throw std::runtime_error("invalid enum string: " + str); \
    } \
    template<> \
    struct serializer<EnumType> { \
        static json to_json(const EnumType& value) { \
            return json(enum_to_string_##EnumType(value)); \
        } \
        static EnumType from_json(const json& j) { \
            return enum_from_string_##EnumType(j.as_string()); \
        } \
    }; \
    }

// 4 values
#define CPPX_ENUM_SERIALIZABLE_4(EnumType, v1, v2, v3, v4) \
    namespace cppx { \
    inline std::string enum_to_string_##EnumType(EnumType value) { \
        switch (value) { \
            CPPX_ENUM_CASE(EnumType, v1) \
            CPPX_ENUM_CASE(EnumType, v2) \
            CPPX_ENUM_CASE(EnumType, v3) \
            CPPX_ENUM_CASE(EnumType, v4) \
            default: return "Unknown"; \
        } \
    } \
    inline EnumType enum_from_string_##EnumType(const std::string& str) { \
        if (str == #v1) return EnumType::v1; \
        if (str == #v2) return EnumType::v2; \
        if (str == #v3) return EnumType::v3; \
        if (str == #v4) return EnumType::v4; \
        throw std::runtime_error("invalid enum string: " + str); \
    } \
    template<> \
    struct serializer<EnumType> { \
        static json to_json(const EnumType& value) { \
            return json(enum_to_string_##EnumType(value)); \
        } \
        static EnumType from_json(const json& j) { \
            return enum_from_string_##EnumType(j.as_string()); \
        } \
    }; \
    }

// 5 values
#define CPPX_ENUM_SERIALIZABLE_5(EnumType, v1, v2, v3, v4, v5) \
    namespace cppx { \
    inline std::string enum_to_string_##EnumType(EnumType value) { \
        switch (value) { \
            CPPX_ENUM_CASE(EnumType, v1) \
            CPPX_ENUM_CASE(EnumType, v2) \
            CPPX_ENUM_CASE(EnumType, v3) \
            CPPX_ENUM_CASE(EnumType, v4) \
            CPPX_ENUM_CASE(EnumType, v5) \
            default: return "Unknown"; \
        } \
    } \
    inline EnumType enum_from_string_##EnumType(const std::string& str) { \
        if (str == #v1) return EnumType::v1; \
        if (str == #v2) return EnumType::v2; \
        if (str == #v3) return EnumType::v3; \
        if (str == #v4) return EnumType::v4; \
        if (str == #v5) return EnumType::v5; \
        throw std::runtime_error("invalid enum string: " + str); \
    } \
    template<> \
    struct serializer<EnumType> { \
        static json to_json(const EnumType& value) { \
            return json(enum_to_string_##EnumType(value)); \
        } \
        static EnumType from_json(const json& j) { \
            return enum_from_string_##EnumType(j.as_string()); \
        } \
    }; \
    }

/**
 * CPPX_ENUM_SERIALIZABLE - Register an enum for serialization (supports 1-5 values)
 * 
 * Usage:
 *   enum class Priority { Low, Medium, High };
 *   CPPX_ENUM_SERIALIZABLE(Priority, Low, Medium, High)
 */
#define CPPX_ENUM_SERIALIZABLE(...) \
    CPPX_GET_MACRO6(__VA_ARGS__, \
        CPPX_ENUM_SERIALIZABLE_5, \
        CPPX_ENUM_SERIALIZABLE_4, \
        CPPX_ENUM_SERIALIZABLE_3, \
        CPPX_ENUM_SERIALIZABLE_2, \
        CPPX_ENUM_SERIALIZABLE_1, \
        invalid \
    )(__VA_ARGS__)

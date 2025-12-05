// cppx.serialization - Macro definitions
// This file provides macros for easy serialization registration

#pragma once

// ============================================================================
// Helper macros for field serialization
// ============================================================================

// 将每个字段序列化到 JSON 对象
#define CPPX_SERIALIZE_FIELD(obj, field) \
    j[#field] = cppx::serializer<std::remove_cvref_t<decltype(obj.field)>>::to_json(obj.field);

// 从 JSON 对象反序列化每个字段 - 宏版本 (用于 CPPX_FOREACH)
#define CPPX_DESERIALIZE_FIELD_IMPL(obj, field) \
    if (g_json_obj.contains(#field)) { \
        obj.field = cppx::serializer<std::remove_cvref_t<decltype(obj.field)>>::from_json(g_json_obj[#field]); \
    }

// 辅助宏：展开变长参数
#define CPPX_EXPAND(...) __VA_ARGS__

// 辅助宏：获取宏参数个数
#define CPPX_ARG_COUNT_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define CPPX_ARG_COUNT(...) CPPX_EXPAND(CPPX_ARG_COUNT_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

// 辅助宏：针对每个字段应用操作
#define CPPX_FOREACH_1(macro, obj, f1) macro(obj, f1)
#define CPPX_FOREACH_2(macro, obj, f1, f2) macro(obj, f1) macro(obj, f2)
#define CPPX_FOREACH_3(macro, obj, f1, f2, f3) macro(obj, f1) macro(obj, f2) macro(obj, f3)
#define CPPX_FOREACH_4(macro, obj, f1, f2, f3, f4) macro(obj, f1) macro(obj, f2) macro(obj, f3) macro(obj, f4)
#define CPPX_FOREACH_5(macro, obj, f1, f2, f3, f4, f5) macro(obj, f1) macro(obj, f2) macro(obj, f3) macro(obj, f4) macro(obj, f5)
#define CPPX_FOREACH_6(macro, obj, f1, f2, f3, f4, f5, f6) macro(obj, f1) macro(obj, f2) macro(obj, f3) macro(obj, f4) macro(obj, f5) macro(obj, f6)
#define CPPX_FOREACH_7(macro, obj, f1, f2, f3, f4, f5, f6, f7) macro(obj, f1) macro(obj, f2) macro(obj, f3) macro(obj, f4) macro(obj, f5) macro(obj, f6) macro(obj, f7)
#define CPPX_FOREACH_8(macro, obj, f1, f2, f3, f4, f5, f6, f7, f8) macro(obj, f1) macro(obj, f2) macro(obj, f3) macro(obj, f4) macro(obj, f5) macro(obj, f6) macro(obj, f7) macro(obj, f8)
#define CPPX_FOREACH_9(macro, obj, f1, f2, f3, f4, f5, f6, f7, f8, f9) macro(obj, f1) macro(obj, f2) macro(obj, f3) macro(obj, f4) macro(obj, f5) macro(obj, f6) macro(obj, f7) macro(obj, f8) macro(obj, f9)
#define CPPX_FOREACH_10(macro, obj, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10) macro(obj, f1) macro(obj, f2) macro(obj, f3) macro(obj, f4) macro(obj, f5) macro(obj, f6) macro(obj, f7) macro(obj, f8) macro(obj, f9) macro(obj, f10)

#define CPPX_FOREACH_IMPL2(count, macro, obj, ...) CPPX_FOREACH_##count(macro, obj, __VA_ARGS__)
#define CPPX_FOREACH_IMPL(count, macro, obj, ...) CPPX_FOREACH_IMPL2(count, macro, obj, __VA_ARGS__)
#define CPPX_FOREACH(macro, obj, ...) CPPX_FOREACH_IMPL(CPPX_ARG_COUNT(__VA_ARGS__), macro, obj, __VA_ARGS__)

// ============================================================================
// Main serialization macro
// ============================================================================

/**
 * Register a type for serialization
 * 
 * Usage:
 *   struct Person {
 *       std::string name;
 *       int age;
 *       std::vector<std::string> hobbies;
 *   };
 *   
 *   CPPX_SERIALIZABLE(Person, name, age, hobbies);
 */
#define CPPX_SERIALIZABLE(Type, ...) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_FOREACH(CPPX_SERIALIZE_FIELD, value, __VA_ARGS__) \
            return j; \
        } \
        \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            const auto& g_json_obj = j_param; \
            CPPX_FOREACH(CPPX_DESERIALIZE_FIELD_IMPL, value, __VA_ARGS__) \
            return value; \
        } \
    }; \
    }

// ============================================================================
// Enum serialization macro
// ============================================================================

// 辅助宏：生成单个枚举值的 case
#define CPPX_ENUM_TO_STRING_CASE(EnumType, value) \
    case EnumType::value: return #value;

#define CPPX_ENUM_FROM_STRING_CASE(EnumType, value) \
    if (str == #value) return EnumType::value;

// 针对每个枚举值生成 case
#define CPPX_ENUM_CASES_1(macro, EnumType, v1) macro(EnumType, v1)
#define CPPX_ENUM_CASES_2(macro, EnumType, v1, v2) macro(EnumType, v1) macro(EnumType, v2)
#define CPPX_ENUM_CASES_3(macro, EnumType, v1, v2, v3) macro(EnumType, v1) macro(EnumType, v2) macro(EnumType, v3)
#define CPPX_ENUM_CASES_4(macro, EnumType, v1, v2, v3, v4) macro(EnumType, v1) macro(EnumType, v2) macro(EnumType, v3) macro(EnumType, v4)
#define CPPX_ENUM_CASES_5(macro, EnumType, v1, v2, v3, v4, v5) macro(EnumType, v1) macro(EnumType, v2) macro(EnumType, v3) macro(EnumType, v4) macro(EnumType, v5)
#define CPPX_ENUM_CASES_6(macro, EnumType, v1, v2, v3, v4, v5, v6) macro(EnumType, v1) macro(EnumType, v2) macro(EnumType, v3) macro(EnumType, v4) macro(EnumType, v5) macro(EnumType, v6)
#define CPPX_ENUM_CASES_7(macro, EnumType, v1, v2, v3, v4, v5, v6, v7) macro(EnumType, v1) macro(EnumType, v2) macro(EnumType, v3) macro(EnumType, v4) macro(EnumType, v5) macro(EnumType, v6) macro(EnumType, v7)
#define CPPX_ENUM_CASES_8(macro, EnumType, v1, v2, v3, v4, v5, v6, v7, v8) macro(EnumType, v1) macro(EnumType, v2) macro(EnumType, v3) macro(EnumType, v4) macro(EnumType, v5) macro(EnumType, v6) macro(EnumType, v7) macro(EnumType, v8)

#define CPPX_ENUM_CASES_IMPL2(count, macro, EnumType, ...) CPPX_ENUM_CASES_##count(macro, EnumType, __VA_ARGS__)
#define CPPX_ENUM_CASES_IMPL(count, macro, EnumType, ...) CPPX_ENUM_CASES_IMPL2(count, macro, EnumType, __VA_ARGS__)
#define CPPX_ENUM_CASES(macro, EnumType, ...) CPPX_ENUM_CASES_IMPL(CPPX_ARG_COUNT(__VA_ARGS__), macro, EnumType, __VA_ARGS__)

/**
 * Register an enum for serialization
 * 
 * Usage:
 *   enum class Priority {
 *       Low,
 *       Medium,
 *       High
 *   };
 *   
 *   CPPX_ENUM_SERIALIZABLE(Priority, Low, Medium, High);
 */
#define CPPX_ENUM_SERIALIZABLE(EnumType, ...) \
    namespace cppx { \
    inline std::string enum_to_string(EnumType value) { \
        switch (value) { \
            CPPX_ENUM_CASES(CPPX_ENUM_TO_STRING_CASE, EnumType, __VA_ARGS__) \
            default: return "Unknown"; \
        } \
    } \
    \
    inline EnumType enum_from_string_##EnumType(const std::string& str) { \
        CPPX_ENUM_CASES(CPPX_ENUM_FROM_STRING_CASE, EnumType, __VA_ARGS__) \
        throw std::runtime_error("invalid enum string: " + str); \
    } \
    \
    template<> \
    struct serializer<EnumType> { \
        static json to_json(const EnumType& value) { \
            return json(enum_to_string(value)); \
        } \
        \
        static EnumType from_json(const json& j) { \
            return enum_from_string_##EnumType(j.as_string()); \
        } \
    }; \
    }


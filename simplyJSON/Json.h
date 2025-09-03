#pragma once
#include "Common.h"
enum JsonTokenEnum
{
	CBRACKETS_OPEN,
	SBRACKETS_OPEN,
	CBRACKETS_CLOSE,
	SBRACKETS_CLOSE,
	COMMA,
	QUOTATION,
	COLON,
	LITERAL
};

struct JsonToken
{
	JsonTokenEnum type;
	std::string value;
};

class JsonValue {
public:
	virtual ~JsonValue() = default;
	virtual std::string asString() const = NULL;
};

class JsonString : JsonValue {
	std::string value;
public:
	JsonString(const std::string& val) : value(val){}
	std::string asString() const override { return "\"" + value + "\""; }
};

class JsonDouble : JsonValue {
	double value;
public:
	JsonDouble(const double val) : value(val){}
	std::string asString() const override { return std::to_string(value); }
};

class JsonBool : JsonValue {
	bool value;
public:
	JsonBool(const bool val) : value(val){}
	std::string asString() const override { return value ? "true" : "false"; }
};

class JsonList : JsonValue {
	std::vector<JsonValue> value;
public:
	JsonList(const std::vector<JsonValue>& val): value(val) {}
	std::string asString();
};

class JsonMap : JsonValue {
	std::unordered_map<std::string, JsonValue> value;
public:
	JsonMap(const std::unordered_map<std::string, JsonValue>& val ): value(val) {}
	std::string asString(); 
};

class Json {
	std::unordered_map<std::string, JsonValue> root;
public:
	Json(std::fstream& file_stream, const std::string& path);
	Json(const std::string& json_as_string);

	JsonValue* operator[] (const std::string& key);

	std::string stringDump();
private:
	std::vector<JsonToken> tokenize(const std::string& json_string);
	void parse(const std::vector<JsonToken>& tokens);
};
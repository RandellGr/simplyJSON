#pragma once
#include "Common.h"

enum JsonType
{
	JSON_BOOL,
	JSON_DOUBLE,
	JSON_STRING,
	JSON_VECTOR,
	JSON_MAP
};

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
	virtual JsonType type() const = NULL;
	
	virtual double getDouble() const { throw std::bad_cast(); }
	virtual bool   getBool() const { throw std::bad_cast(); }
	virtual std::string getString() const { throw std::bad_cast(); }
	virtual std::vector<std::shared_ptr<JsonValue>> getList() const { throw std::bad_cast(); }
	virtual const std::unordered_map<std::string, std::shared_ptr<JsonValue>>& getMap() const { throw std::bad_cast(); }

	virtual std::string* getStringPtr() const { throw std::bad_cast(); }
	virtual std::vector<std::shared_ptr<JsonValue>>* getListPtr() const { throw std::bad_cast(); }
	virtual std::unordered_map<std::string, std::shared_ptr<JsonValue>>* getMapPtr() const { throw std::bad_cast(); }
};

class JsonString : public JsonValue {
	std::string value;
	std::string* value_ptr = nullptr;
public:
	JsonString(const std::string& val) : value(val), value_ptr(&value){}
	std::string asString() const override { return "\"" + value + "\""; }
	JsonType type() const override { return JSON_STRING; }
	std::string getString() const override { return value; }
	std::string* getStringPtr() const override { return value_ptr; }
};

class JsonDouble : public JsonValue {
	double value;
public:
	JsonDouble(const double val) : value(val){}
	std::string asString() const override { return std::to_string(value); }
	JsonType type() const override { return JSON_DOUBLE; }
	double getDouble() const override { return value; }
};

class JsonBool : public JsonValue {
	bool value;
public:
	JsonBool(const bool val) : value(val){}
	std::string asString() const override { return value ? "true" : "false"; }
	JsonType type() const override { return JSON_BOOL; }
	bool getBool() const override { return value; }
};

class JsonList : public JsonValue {
	std::vector<std::shared_ptr<JsonValue>> value;
	std::vector<std::shared_ptr<JsonValue>>* value_ptr;
public:
	JsonList(const std::vector<std::shared_ptr<JsonValue>>& val): value(val), value_ptr(&value) {}
	JsonList(): value(), value_ptr(&value) {}
	std::string asString() const override { return "list placeholder"; };
	JsonType type() const override { return JSON_VECTOR; }
	std::vector<std::shared_ptr<JsonValue>> getList() const override { return value; }
	std::vector<std::shared_ptr<JsonValue>>* getListPtr() const override { return value_ptr; }
};

class JsonMap : public JsonValue {
	std::unordered_map<std::string, std::shared_ptr<JsonValue>> value;
	std::unordered_map<std::string, std::shared_ptr<JsonValue>>* value_ptr;
public:
	JsonMap(const std::unordered_map<std::string, std::shared_ptr<JsonValue>>& val ): value(val), value_ptr(&value) {}
	JsonMap(): value(0), value_ptr(&value) {}
	std::string asString() const override { return "map placeholder"; }
	JsonType type() const override { return JSON_MAP; }
	const std::unordered_map<std::string, std::shared_ptr<JsonValue>>& getMap() const override { return value; }
	std::unordered_map<std::string, std::shared_ptr<JsonValue>>* getMapPtr() const override { return value_ptr; }
};

class Json {
	std::unordered_map<std::string, std::shared_ptr<JsonValue>> root;
public:
	Json(std::fstream& file_stream, const std::string& path);
	Json(const std::string& json_as_string);

	JsonValue* operator[] (const std::string& key);

	std::string stringDump();
private:
	std::vector<JsonToken> tokenize(const std::string& json_string);
	void parse(const std::vector<JsonToken>& tokens);
};
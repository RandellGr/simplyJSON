#pragma once
#include "Common.h"
namespace smpj {
	enum JsonType
	{
		JSON_BOOL,
		JSON_DOUBLE,
		JSON_STRING,
		JSON_VECTOR,
		JSON_MAP,
		JSON_NULL
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

	enum TokenContext {
		CTX_KEY,
		CTX_VALUE,
		CTX_OBJECT,
		CTX_LIST,
		CTX_STRING
	};

	struct JsonToken
	{
		JsonTokenEnum type;
		std::string value;
	};

	struct Error {
		std::string what;
		bool is_present;
	};

	class JsonValue {
	public:
		virtual ~JsonValue() = default;
		virtual std::string asString(int offset = 0) const = NULL;
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

	class JsonNull : public JsonValue {
	public:
		JsonNull() {};
		std::string asString(int offset = 0) const override { return "null"; }
		JsonType type() const override { return JSON_NULL; }
	};

	class JsonString : public JsonValue {
		std::string value;
		std::string* value_ptr = nullptr;
	public:
		JsonString(const std::string& val) : value(val), value_ptr(&value) {}
		std::string asString(int offset = 0) const override { return "\"" + value + "\""; }
		JsonType type() const override { return JSON_STRING; }
		std::string getString() const override { return value; }
		std::string* getStringPtr() const override { return value_ptr; }
	};

	class JsonDouble : public JsonValue {
		double value;
	public:
		JsonDouble(const double val) : value(val) {}
		std::string asString(int offset = 0) const override;
		JsonType type() const override { return JSON_DOUBLE; }
		double getDouble() const override { return value; }
	};

	class JsonBool : public JsonValue {
		bool value;
	public:
		JsonBool(const bool val) : value(val) {}
		std::string asString(int offset = 0) const override { return value ? "true" : "false"; }
		JsonType type() const override { return JSON_BOOL; }
		bool getBool() const override { return value; }
	};

	class JsonList : public JsonValue {
		std::vector<std::shared_ptr<JsonValue>> value;
		std::vector<std::shared_ptr<JsonValue>>* value_ptr;
	public:
		JsonList(const std::vector<std::shared_ptr<JsonValue>>& val) : value(val), value_ptr(&value) {}
		JsonList() : value(), value_ptr(&value) {}
		JsonType type() const override { return JSON_VECTOR; }
		std::string asString(int offset = 0) const override;
		std::vector<std::shared_ptr<JsonValue>> getList() const override { return value; }
		std::vector<std::shared_ptr<JsonValue>>* getListPtr() const override { return value_ptr; }
	};

	class JsonMap : public JsonValue {
		std::unordered_map<std::string, std::shared_ptr<JsonValue>> value;
		std::unordered_map<std::string, std::shared_ptr<JsonValue>>* value_ptr;
	public:
		JsonMap(const std::unordered_map<std::string, std::shared_ptr<JsonValue>>& val) : value(val), value_ptr(&value) {}
		JsonMap() : value(0), value_ptr(&value) {}
		JsonType type() const override { return JSON_MAP; }
		std::string asString(int offset = 0) const override;
		const std::unordered_map<std::string, std::shared_ptr<JsonValue>>& getMap() const override { return value; }
		std::unordered_map<std::string, std::shared_ptr<JsonValue>>* getMapPtr() const override { return value_ptr; }
	};

	class Json {
		std::shared_ptr<JsonMap> root;
	public:
		Json(std::fstream& file_stream, const std::string& path);
		Json(const std::string& json_as_string);

		std::shared_ptr<JsonValue>& operator[] (const std::string& key);
		const std::shared_ptr<JsonValue>& operator[] (const std::string& key) const;

		void writeToFile(std::fstream& file_stream, const std::string& path);

		std::string stringDump() const {
			return root->asString(0);
		};
		
	private:
		std::vector<JsonToken> tokenize(const std::string& json_string);
		void parse(const std::vector<JsonToken>& tokens);
		Error validate(const std::vector<JsonToken>& tokens);
	};

}

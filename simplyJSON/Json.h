#pragma once
#include "Common.h"
#include "Template.h"

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

	enum JsonParseErrors {
		JSON_NULL_EX,
		JSON_EMPTY,
		JSON_UNEXPECTED_SYMBOL,
		JSON_UNEXPECTED_VALUE,
		JSON_MISSING_SYMBOL,
		JSON_MISSING_VALUE,
		JSON_INVALID_LITERAL,
		JSON_INVALID_CONTEXT,
		JSON_INVALID_KEY,
		JSON_INVALID_STRING,
		JSON_OK
	};

	struct JsonToken
	{
		JsonTokenEnum type;
		std::string value;
		size_t line;
		size_t column;
	};

	struct ParseError {
	public:
		ParseError() : content(""), id(JSON_NULL_EX), position{0,0} {}
		ParseError(JsonParseErrors _id, std::string what, int line = 0, int column = 0)
			: content(what), id(_id), position{line, column} {
		}
		JsonParseErrors get_id() const { return id; }
		std::string info() const 
			{ return "ParseError id: " + std::to_string(id) + " - " + content + " at: " + std::to_string(position[0]) + " " + std::to_string(position[1]); }
	private:
		JsonParseErrors id;
		std::string content;
		int position[2];
	};

	class JsonValue {
	public:
		virtual ~JsonValue() = default;
		virtual std::string asString(int offset = 0) const = NULL;
		virtual JsonType type() const = NULL;
		virtual std::shared_ptr<JsonValue> clone() const = 0;

		virtual double getDouble() const { throw std::bad_cast(); }
		virtual bool   getBool() const { throw std::bad_cast(); }
		virtual std::string getString() const { throw std::bad_cast(); }
		virtual std::vector<std::shared_ptr<JsonValue>> getList() const { throw std::bad_cast(); }
		virtual const std::unordered_map<std::string, std::shared_ptr<JsonValue>>& getMap() const { throw std::bad_cast(); }

		virtual std::string* getStringPtr() const { throw std::bad_cast(); }
		virtual std::vector<std::shared_ptr<JsonValue>>* getListPtr() const { throw std::bad_cast(); }
		virtual std::unordered_map<std::string, std::shared_ptr<JsonValue>>* getMapPtr() const { throw std::bad_cast(); }

		virtual std::shared_ptr<JsonValue>& operator[](const std::string& key) { throw std::runtime_error("no [ string ] opertaor for this json value type"); }
		virtual const std::shared_ptr<JsonValue>& operator[](const std::string& key) const { throw std::runtime_error("no [ stirng ] opertaor for this json value type"); }
		virtual std::shared_ptr<JsonValue>& operator[](size_t index) { throw std::runtime_error("no [ index ] opertaor for this json value type"); }
		virtual const std::shared_ptr<JsonValue>& operator[](size_t index) const { throw std::runtime_error("no [ index ] opertaor for this json value type"); }
	};

	class JsonNull : public JsonValue {
	public:
		JsonNull() {};
		std::string asString(int offset = 0) const override { return "null"; }
		JsonType type() const override { return JSON_NULL; }
		std::shared_ptr<JsonValue> clone() const override { return std::make_shared<JsonNull>(); }
	};

	class JsonString : public JsonValue {
		std::string value;
		std::string* value_ptr = nullptr;
	public:
		JsonString(const std::string& val) : value(val), value_ptr(&value) {}
		std::string asString(int offset = 0) const override;
		JsonType type() const override { return JSON_STRING; }
		std::shared_ptr<JsonValue> clone() const override { return std::make_shared<JsonString>(value); }
		std::string getString() const override { return value; }
		std::string* getStringPtr() const override { return value_ptr; }
	};

	class JsonDouble : public JsonValue {
		double value;
	public:
		JsonDouble(const double val) : value(val) {}
		std::string asString(int offset = 0) const override;
		JsonType type() const override { return JSON_DOUBLE; }
		std::shared_ptr<JsonValue> clone() const override { return std::make_shared<JsonDouble>(value); }
		double getDouble() const override { return value; }
	};

	class JsonBool : public JsonValue {
		bool value;
	public:
		JsonBool(const bool val) : value(val) {}
		std::string asString(int offset = 0) const override { return value ? "true" : "false"; }
		JsonType type() const override { return JSON_BOOL; }
		std::shared_ptr<JsonValue> clone() const override { return std::make_shared<JsonBool>(value); }
		bool getBool() const override { return value; }
	};

	class JsonList : public JsonValue {
		std::vector<std::shared_ptr<JsonValue>> value;
		std::vector<std::shared_ptr<JsonValue>>* value_ptr;
	public:
		JsonList(const std::vector<std::shared_ptr<JsonValue>>& val) : value(val), value_ptr(&value) {}
		JsonList() : value(), value_ptr(&value) {}
		JsonType type() const override { return JSON_VECTOR; }
		std::shared_ptr<JsonValue> clone() const override;
		std::string asString(int offset = 0) const override;
		std::vector<std::shared_ptr<JsonValue>> getList() const override { return value; }
		std::vector<std::shared_ptr<JsonValue>>* getListPtr() const override { return value_ptr; }
		std::shared_ptr<JsonValue>& operator[](size_t index) override;
		const std::shared_ptr<JsonValue>& operator[](size_t index) const override;
	};

	class JsonMap : public JsonValue {
		std::unordered_map<std::string, std::shared_ptr<JsonValue>> value;
		std::unordered_map<std::string, std::shared_ptr<JsonValue>>* value_ptr;
	public:
		JsonMap(const std::unordered_map<std::string, std::shared_ptr<JsonValue>>& val) : value(val), value_ptr(&value) {}
		JsonMap() : value(0), value_ptr(&value) {}
		JsonType type() const override { return JSON_MAP; }
		std::shared_ptr<JsonValue> clone() const override;
		std::string asString(int offset = 0) const override;
		const std::unordered_map<std::string, std::shared_ptr<JsonValue>>& getMap() const override { return value; }
		std::unordered_map<std::string, std::shared_ptr<JsonValue>>* getMapPtr() const override { return value_ptr; }
		std::shared_ptr<JsonValue>& operator[](const std::string& index) override;
		const std::shared_ptr<JsonValue>& operator[](const std::string& index) const override;
	};

	class Json {
		std::shared_ptr<JsonValue> root;
	public:
		Json(std::fstream& file_stream, const std::string& path, ParseError* ParseError_ptr = nullptr);
		Json(const std::string& json_as_string, ParseError* ParseError_ptr = nullptr);
		Json(const char* string_literal, ParseError* ParseError_ptr = nullptr);
		Json();
		Json(const Json& other);
		Json(Json&& other) noexcept;

		std::shared_ptr<JsonValue>& operator[] (const std::string& key);
		const std::shared_ptr<JsonValue>& operator[] (const std::string& key) const;

		std::shared_ptr<JsonValue>& operator[] (size_t index);
		const std::shared_ptr<JsonValue>& operator[] (size_t index) const;

		void writeToFile(std::fstream& file_stream, const std::string& path);

		std::string stringDump() const {
			return root->asString(0);
		};
		
	private:
		std::vector<JsonToken> tokenize(const std::string& json_string, ParseError* ParseError_ptr = nullptr);
		std::vector<JsonToken> streaming_tokenize(std::fstream& filestream, ParseError* ParseError_ptr = nullptr);
		void parse(const std::vector<JsonToken>& tokens);
		bool validate(const std::vector<JsonToken>& tokens, ParseError* ParseError_ptr = nullptr);
	};

	template<typename Type>
	std::shared_ptr<JsonValue> makeJson(Type&& input) {
		using Decayed = std::decay_t<Type>;

		if constexpr (std::is_same_v<Decayed, std::shared_ptr<JsonValue>>) {
			return input;
		}
		else if constexpr (std::is_base_of_v<JsonValue, Decayed>) {
			return input.clone();
		}
		else if constexpr (std::is_same_v<Decayed, bool>) {
			return std::make_shared<JsonBool>(input);
		}
		else if constexpr (std::is_arithmetic_v<Decayed>) {
			return std::make_shared<JsonDouble>(static_cast<double>(input));
		}
		else if constexpr (std::is_same_v<Decayed, const char*>) {
			return std::make_shared<JsonString>(input);
		}
		else if constexpr (std::is_same_v<Decayed, std::string>) {
			return std::make_shared<JsonString>(input);
		}
		else if constexpr (is_vector<Decayed>::value) {
			auto list = std::make_shared<JsonList>();
			for (auto&& element : input) {
				list->getListPtr()->push_back(makeJson(std::forward<decltype(element)>(element)));
			}
			return list;
		}
		else if constexpr (is_umap<Decayed>::value) {
			auto object = std::make_shared<JsonMap>();
			for (auto&& [key, val] : input) {
				object->getMapPtr()->emplace(key, makeJson(std::forward<decltype(val)>(val)));
			}
			return object;
		}
		else
		{
			static_assert(always_false<Type>, "Type is not JSON-convertible");
		}

	}
}
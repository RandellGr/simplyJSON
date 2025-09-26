#pragma once
#include "Json.h"

using namespace smpj;

bool is_json_number(const std::string& input) {
	if (input.empty()) return false;
	size_t i = 0;
	size_t input_size = input.size();

	if (input[i] == '-') {
		i++; if (i == input_size) return false;
	}

	if (input[i] == '0') {
		i++; if (i < input_size && isdigit(input[i])) return false;
	}
	else if (isdigit(input[i])) {
		if (input[i] == '0') return false;
		while (i < input_size && isdigit(input[i])) i++;
	}
	else {
		return false;
	}

	if (i < input_size && input[i] == '.') {
		i++;
		if (i == input_size || !isdigit(input[i])) return false;
		while (i < input_size && isdigit(input[i])) i++;
	}

	if (i < input_size && (input[i] == 'e' || input[i] == 'E')) {
		i++;
		if (i < input_size && (input[i] == '+' || input[i] == '-')) i++;
		if (i == input_size || !isdigit(input[i])) return false; 
		while (i < input_size && isdigit(input[i])) i++; 
	}

	return i == input_size;
}

std::string parse_json_string(const std::string& input, size_t& it) {
	++it;
	std::string working_buffer;
	while (it < input.size()) {
		if (input[it] == '"') return working_buffer;
		char current = input[it++];
		if (current == '\n' || current == '\r') throw std::runtime_error("\\n and \\r are not allowed in string");
		if (current == '\\') {
			if (it >= input.size()) throw std::runtime_error("Escape sequence at the end of the string at position " + it);
			char escaped = input[it++];
			switch (escaped) {
			case '"':	working_buffer += '"'; break;
			case '\\':	working_buffer += '\\'; break;
			case '/':	working_buffer += '/'; break;
			case 'b':	working_buffer += '\b'; break;
			case 'f':	working_buffer += '\f'; break;
			case 'n':	working_buffer += '\n'; break;
			case 'r':	working_buffer += '\r'; break;
			case 't':	working_buffer += '\t'; break;
			case 'u':	throw std::runtime_error("Unicode escaped symbol is not supported");
			default:	throw std::runtime_error("Invalid escape \\" + escaped);
			}
		}
		else working_buffer += current;
	}
	throw std::runtime_error("Unterminated string");
}

Json::Json(std::fstream& filestream, const std::string& path) 
{
	size_t file_end;
	std::string file_content;
	std::vector<JsonToken> tokens;

	filestream.open(path, std::ios::in | std::ios::binary | std::ios::ate);
	if (!filestream.is_open()) throw std::runtime_error("could not open filestream at " + path + "\n");
	file_end = filestream.tellg();
	filestream.seekg(0, std::ios::beg);
	file_content.resize(file_end);
	filestream.read(file_content.data(), file_end);
	filestream.close();

	tokens = tokenize(file_content);
	auto error = validate(tokens);
	if (error.is_present) throw std::runtime_error("JSON synthax error: " + error.what);
	if (tokens[0].type == CBRACKETS_OPEN)	   root = std::make_shared<JsonMap>();
	else if (tokens[0].type == SBRACKETS_OPEN) root = std::make_shared<JsonList>();
	parse(tokens);
}

Json::Json(const std::string& json_string)
{
	std::vector<JsonToken> tokens;

	tokens = tokenize(json_string);
	auto error = validate(tokens);
	if (error.is_present) throw std::runtime_error("JSON synthax error: " + error.what);
	if (tokens[0].type == CBRACKETS_OPEN)	   root = std::make_shared<JsonMap>();
	else if (tokens[0].type == SBRACKETS_OPEN) root = std::make_shared<JsonList>();
	parse(tokens);
}

Json::Json(const char* string_literal) 
{
	std::vector<JsonToken> tokens;

	tokens = tokenize(std::string(string_literal));
	auto error = validate(tokens);
	if (error.is_present) throw std::runtime_error("JSON synthax error: " + error.what);
	if (tokens[0].type == CBRACKETS_OPEN)	   root = std::make_shared<JsonMap>();
	else if (tokens[0].type == SBRACKETS_OPEN) root = std::make_shared<JsonList>();
	parse(tokens);
}

Json::Json(const Json& other) {
	if(other.root->type() == JSON_MAP)
		root = std::dynamic_pointer_cast<JsonMap>(other.root->clone());
	else 
		root = std::dynamic_pointer_cast<JsonList>(other.root->clone());
}

Json::Json(Json&& other) noexcept
	: root(std::move(other.root))
{
	other.root = std::make_shared<JsonMap>();
}

Json::Json()
	: root(std::make_shared<JsonMap>()){}

std::vector<JsonToken> Json::tokenize(const std::string& json_string) {
	std::vector<JsonToken> tokens;

	for (size_t i = 0; i != json_string.size(); ++i) {
		switch (json_string[i]) {
		case '{':
			tokens.push_back({ CBRACKETS_OPEN, "" });
			break;
		case '}':
			tokens.push_back({ CBRACKETS_CLOSE, "" });
			break;
		case '[':
			tokens.push_back({ SBRACKETS_OPEN, "" });
			break;
		case ']':
			tokens.push_back({ SBRACKETS_CLOSE, "" });
			break;
		case ',':
			tokens.push_back({ COMMA, "" });
			break;
		case '"':
			tokens.push_back({ QUOTATION, "" });
			tokens.push_back({ LITERAL, parse_json_string(json_string, i) });
			tokens.push_back({ QUOTATION, "" });
			break;
		case ':':
			tokens.push_back({ COLON, "" });
			break;
		case ' ' :
		case '\n':
		case '\r':
		case '\t':
			break;
		default:
			size_t it;
			it = json_string.find_first_of("{}[],\":\n\r\t ", i);
			if (it == std::string::npos) it = json_string.size();
			tokens.push_back({ LITERAL, std::string(json_string.begin() + i, json_string.begin() + it)});
			i = it - 1;
			break;
		}
	}
	return tokens; 
}

Error Json::validate(const std::vector<JsonToken>& tokens) {
	if (tokens.empty()) return { "empty input", true };
	if (tokens[0].type != CBRACKETS_OPEN && tokens[0].type != SBRACKETS_OPEN)
		return { "top level object is not found" , true };

	std::stack<TokenContext> context;
	bool string_flag = false;

	size_t token_position = 0;
	JsonToken prev_token;

	for (JsonToken token : tokens) {
		switch (token.type) {
		case CBRACKETS_OPEN:
			context.push(CTX_OBJECT);
			break;
		case CBRACKETS_CLOSE:
			if (prev_token.type != LITERAL && prev_token.type != QUOTATION &&
				prev_token.type != CBRACKETS_CLOSE && prev_token.type != SBRACKETS_CLOSE &&
				prev_token.type != CBRACKETS_OPEN)
				return { "value is not found at token: " + std::to_string(token_position), true };
			if (context.top() == CTX_VALUE) context.pop();
			if (context.top() != CTX_OBJECT) return { "object closing without opening at token: " + std::to_string(token_position), true };
			context.pop();
			break;
		case SBRACKETS_OPEN:
			context.push(CTX_LIST);
			break;
		case SBRACKETS_CLOSE:
			if (prev_token.type != LITERAL && prev_token.type != QUOTATION &&
				prev_token.type != CBRACKETS_CLOSE && prev_token.type != SBRACKETS_CLOSE &&
				prev_token.type != SBRACKETS_OPEN)
				return { "value is not found at token: " + std::to_string(token_position), true };
			if (prev_token.type == COMMA) return { "trailing comma in list at token: " + std::to_string(token_position), true };
			if (context.top() != CTX_LIST) return { "list closing without opening at token: " + std::to_string(token_position), true };
			context.pop();
			break;
		case QUOTATION: {
			auto top = context.top();
			if (!string_flag) {
				if (prev_token.type == QUOTATION) {
					return { "unexpected string at token: " + std::to_string(token_position), true };
				}
				context.push(CTX_STRING);
				string_flag = !string_flag;
				break;
			}
			if (top != CTX_STRING) return { "closing string literal without opening at token: " + std::to_string(token_position), true };
			context.pop();
			string_flag = !string_flag;
			break;
		}
		case COMMA: {
			auto top = context.top();
			if (prev_token.type != LITERAL && prev_token.type != QUOTATION &&
				prev_token.type != CBRACKETS_CLOSE && prev_token.type != SBRACKETS_CLOSE) 
				return { "value is not found at token: " + std::to_string(token_position), true };
			if (top != CTX_VALUE && top != CTX_LIST) return { "inproper comma placement at token: " + std::to_string(token_position), true };
			if (top == CTX_VALUE) {
				context.pop();
				context.push(CTX_KEY);
			}
			break;
		}
		case COLON: {
			auto top = context.top();
			if (prev_token.type != QUOTATION) return { "key is not a string at token: " + std::to_string(token_position), true };
			if (top != CTX_KEY && top != CTX_OBJECT) return { "inproper colon placement at token: " + std::to_string(token_position), true };
			if (top == CTX_KEY) context.pop();
			context.push(CTX_VALUE);
			break;
		}
		case LITERAL:
			if (!string_flag && 
			   (token.value != "false" && token.value != "true" &&
				token.value != "null"  && !is_json_number(token.value)))
					return { "invalid literal: " + token.value +" at token: " + std::to_string(token_position), true };
			break;
		}
		token_position++;
		prev_token = token;
	}
	if (context.empty()) {
		return { "", false };
	}
	else {
		return { "no closing bracket found!", true };
	}
}

void Json::parse(const std::vector<JsonToken>& tokens) {
	using json_map = std::unordered_map<std::string, std::shared_ptr<JsonValue>>;
	using json_list = std::vector<std::shared_ptr<JsonValue>>;
	using json_stack = std::vector<std::shared_ptr<JsonValue>>;
	using json_value_sptr = std::shared_ptr<JsonValue>;

	json_map*				current_map_ptr		= nullptr;
	json_list*				list_ptr			= nullptr;
	json_stack				json_ptrs_stack;
	json_value_sptr			inner_value_ptr		= nullptr;
	json_value_sptr			last_value_ptr		= nullptr;
	json_value_sptr			new_value_ptr		= nullptr;

	std::string				last_literal;
	std::string				last_key;
	bool					string_flag			= false;
	JsonToken				prev_token;

	if (root->type() == JSON_MAP)			current_map_ptr = root->getMapPtr();
	else if (root->type() == JSON_VECTOR)	list_ptr = root->getListPtr();

	auto recompute_current_map = [&]() {
		current_map_ptr = nullptr;
		for (int i = (int)json_ptrs_stack.size() - 1; i >= 0; --i) {
			if (json_ptrs_stack[i]->type() == JSON_MAP) {
				current_map_ptr = json_ptrs_stack[i]->getMapPtr();
				break;
			}
		}
	};

	for (JsonToken token : tokens) {
		switch (token.type) {
		case CBRACKETS_OPEN: {
			if (json_ptrs_stack.empty()) {
				root = std::make_shared<JsonMap>();
				json_ptrs_stack.push_back(root);
				current_map_ptr = root->getMapPtr();
			}
			else if (json_ptrs_stack.back()->type() == JSON_VECTOR) {
				list_ptr = json_ptrs_stack.back()->getListPtr();
				list_ptr->push_back(std::make_shared<JsonMap>());
				inner_value_ptr = list_ptr->back();
				json_ptrs_stack.push_back(inner_value_ptr);
				current_map_ptr = inner_value_ptr->getMapPtr();
			}
			else {
				(*current_map_ptr)[last_key] = std::make_shared<JsonMap>();
				inner_value_ptr = (*current_map_ptr)[last_key];
				json_ptrs_stack.push_back(inner_value_ptr);
				current_map_ptr = inner_value_ptr->getMapPtr();
			}
			break;
		}
		case CBRACKETS_CLOSE: {
			if (prev_token.type == QUOTATION || prev_token.type == LITERAL) {
				(*current_map_ptr)[last_key] = last_value_ptr;
			}

			if (!json_ptrs_stack.empty()) {
				json_ptrs_stack.pop_back();
			}
			recompute_current_map();
			break;
		}
		case SBRACKETS_OPEN: {
			new_value_ptr = std::make_shared<JsonList>();
			if (json_ptrs_stack.empty()) {
				root = new_value_ptr;
				json_ptrs_stack.push_back(root);
			}
			else if (json_ptrs_stack.back()->type() == JSON_VECTOR) {
				list_ptr = json_ptrs_stack.back()->getListPtr();
				list_ptr->push_back(new_value_ptr);
				inner_value_ptr = list_ptr->back();
				json_ptrs_stack.push_back(inner_value_ptr);
			}
			else {
				(*current_map_ptr)[last_key] = new_value_ptr;
				inner_value_ptr = (*current_map_ptr)[last_key];
				json_ptrs_stack.push_back(inner_value_ptr);
			}

			break;
		}
		case SBRACKETS_CLOSE: {
			if (prev_token.type == LITERAL || prev_token.type == QUOTATION) {
				list_ptr = json_ptrs_stack.back()->getListPtr();
				list_ptr->push_back(last_value_ptr);
			}
			if (!json_ptrs_stack.empty()) json_ptrs_stack.pop_back();
			recompute_current_map();
			break;
		}
		case COMMA: {
			if (prev_token.type == SBRACKETS_CLOSE || prev_token.type == CBRACKETS_CLOSE) break;
			if (!json_ptrs_stack.empty() && json_ptrs_stack.back()->type() == JSON_VECTOR) {
				list_ptr = json_ptrs_stack.back()->getListPtr();
				list_ptr->push_back(last_value_ptr);
			}
			else if (current_map_ptr) {
				(*current_map_ptr)[last_key] = last_value_ptr;
			}
			break;
		}
		case COLON:
			last_key = last_literal;
			break;

		case QUOTATION:
			string_flag = !string_flag;
			break;

		case LITERAL: {
			last_literal = token.value;
			if (string_flag) {
				last_value_ptr = std::make_shared<JsonString>(token.value);
			}
			else if (token.value == "true") {
				last_value_ptr = std::make_shared<JsonBool>(true);
			}
			else if (token.value == "false") {
				last_value_ptr = std::make_shared<JsonBool>(false);
			}
			else if (token.value == "null") {
				last_value_ptr = std::make_shared<JsonNull>();
			}
			else {
				last_value_ptr = std::make_shared<JsonDouble>(std::stod(token.value));
			}
			break;
		}
		}

		prev_token = token;
	}
}

void Json::writeToFile(std::fstream& file_stream, const std::string& path) {
	file_stream.open(path, std::ios::out | std::ios::binary);
	if (!file_stream.is_open()) throw std::runtime_error("could not open filestream at " + path + "\n");
	std::string contents = stringDump();
	file_stream.write(contents.data(), contents.size());
	file_stream.close();
}

std::shared_ptr<JsonValue>& Json::operator[] (const std::string& key) {
	if (root->type() != JSON_MAP) throw std::runtime_error("invalid operator usage for JsonMap top level object, use strings only");
	return (*root->getMapPtr())[key];
}
const std::shared_ptr<JsonValue>& Json::operator[] (const std::string& key) const {
	if (root->type() != JSON_MAP) throw std::runtime_error("invalid operator usage for JsonMap top level object, use strings only");
	auto& map = *root->getMapPtr();
	auto it = map.find(key);
	if (it == map.end()) throw std::out_of_range("Key not found in JSON object");
	return it->second;
}
std::shared_ptr<JsonValue>& Json::operator[] (size_t index) {
	if (root->type() != JSON_VECTOR) throw std::runtime_error("invalid operator usage for JsonList top level object, use integers only");
	return (*root->getListPtr())[index];
}
const std::shared_ptr<JsonValue>& Json::operator[] (size_t index) const {
	if (root->type() != JSON_VECTOR) throw std::runtime_error("invalid operator usage for JsonList top level object, use integers only");
	auto& list = *root->getListPtr();
	if (index > list.size()) throw std::out_of_range("Index out of range");
	return list[index];
}

std::shared_ptr<JsonValue>& JsonList::operator[](size_t index) {
	if (index >= value.size()) throw std::runtime_error("index is out of bounds");
	return value[index];
}
const std::shared_ptr<JsonValue>& JsonList::operator[](size_t index) const {
	if (index >= value.size()) throw std::runtime_error("index is out of bounds");
	return value[index];
}
std::shared_ptr<JsonValue>& JsonMap::operator[] (const std::string& key) {
	return value[key];
}
const std::shared_ptr<JsonValue>& JsonMap::operator[] (const std::string& key) const {
	auto it = value.find(key);
	if (it == value.end()) throw std::out_of_range("Key not found in JSON object");
	return it->second;
}

std::string JsonList::asString(int offset) const {
	std::string output = "[";
	bool contains_primitives = true;
	char delimiter;
	std::string indent;
	for (auto& val : value) {
		if (val->type() == JSON_MAP || val->type() == JSON_VECTOR) {
			contains_primitives = false;
			break;
		}
	}
	if (contains_primitives) {
		delimiter = ' ';
		indent = "";
	} 
	else {
		delimiter = '\n';
		indent = std::string((offset + 1) * 4, ' ');
	}
	if (!value.empty()) {
		output += delimiter;
		for (size_t i = 0; i < value.size(); ++i) {
			output += indent + value[i]->asString(offset + 1);
			if (i + 1 < value.size()) output += ",";
			output += delimiter;
		}
		if(!contains_primitives) output += std::string(offset * 4, ' ');
	}
	output += "]";
	return output;
}

std::string JsonMap::asString(int offset) const {
	std::string output = "{";
	if (!value.empty()) {
		output += "\n";
		std::string indent((offset + 1) * 4, ' ');

		size_t count = 0;
		for (auto& [key, val] : value) {
			output += indent + "\"" + key + "\" : " + val->asString(offset + 1);
			if (++count < value.size()) output += ",";
			output += "\n";
		}
		output += std::string(offset * 4, ' ');
	}
	output += "}";
	return output;
}

std::string JsonDouble::asString(int offset) const {
	std::string output = std::to_string(value);
	auto last_valid = output.find_last_not_of('0') + 1;
	if (last_valid != std::string::npos) {
		output.erase(output.begin() + last_valid , output.end());
		if (output.back() == '.') output.pop_back();
	}
	return output;
}

std::string JsonString::asString(int offset) const {
	std::string output = "\"";
	for (char current : value) {
		switch (current) {
		case '\b': output += "\\b"; break;
		case '\f': output += "\\f"; break;
		case '\n': output += "\\n"; break;
		case '\r': output += "\\r"; break;
		case '\t': output += "\\t"; break;
		case '"':  output += "\\\""; break;
		case '\\': output += "\\\\"; break;
		case '/':  output += "\\/"; break;
		default:   output += current;
		}
	}
	output += "\"";
	return output;
}

std::shared_ptr<JsonValue> JsonList::clone() const {
	auto copy = std::make_shared<JsonList>();
	for (auto& element : value) {
		copy->value.push_back(element->clone());
	}
	return copy;
}

std::shared_ptr<JsonValue> JsonMap::clone() const {
	auto copy = std::make_shared<JsonMap>();
	for (auto& [key, val] : value) {
		copy->value.emplace(key, val->clone());
	}
	return copy;
}
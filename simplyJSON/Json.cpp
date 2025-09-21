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

Json::Json(std::fstream& filestream, const std::string& path) {
	root = std::make_shared<JsonMap>();

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
	parse(tokens);
}

Json::Json(const std::string& json_string) {
	root = std::make_shared<JsonMap>();
	std::vector<JsonToken> tokens;

	tokens = tokenize(json_string);
	parse(tokens);
}

std::vector<JsonToken> Json::tokenize(const std::string& json_string) {
	std::vector<JsonToken> tokens;
	std::string working_buffer;

	for (int i = 0; i != json_string.size(); ++i) {
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
			if (tokens.back().type == QUOTATION) {
				tokens.push_back({ LITERAL, ""});
				tokens.push_back({ QUOTATION, "" });
				break;
			}
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
			auto it = json_string.find_first_of("{}[],\":\n\r\t", i);
			if (it == std::string::npos) it = json_string.size();
			working_buffer.resize(it - i);
			std::copy(json_string.begin() + i, json_string.begin() + it, working_buffer.data());
			i = it - 1;
			tokens.push_back({ LITERAL, working_buffer });
			working_buffer.clear();
			break;
		}
	}
	return tokens; 
}

Error Json::validate(const std::vector<JsonToken>& tokens) {
	if (tokens[0].type != CBRACKETS_OPEN) return { "top level object is not found" , true };
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
			// нужен патч для кейса {"a" "a"}
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
	using json_stack = std::stack<std::shared_ptr<JsonValue>>;
	using json_value_sptr = std::shared_ptr<JsonValue>;

	json_map*				current_map_ptr		= root->getMapPtr();
	json_stack				json_ptrs_stack;
	json_value_sptr			inner_value_ptr		= nullptr;
	json_value_sptr			last_value_ptr		= nullptr;
	json_value_sptr			new_value_ptr		= nullptr;
	json_list*				list_ptr			= nullptr;

	std::string				last_literal; 
	std::string				last_key;

	bool					string_flag			= false;
	JsonToken				prev_token;

	for (JsonToken token : tokens) {
		switch (token.type) {
		case CBRACKETS_OPEN: {
			if (json_ptrs_stack.empty()) {
				json_ptrs_stack.push(root);
			}
			else if (json_ptrs_stack.top()->type() == JSON_VECTOR) {
				//top of the stack is json list
				list_ptr = json_ptrs_stack.top()->getListPtr();
				list_ptr->push_back(std::make_shared<JsonMap>());
				inner_value_ptr = list_ptr->back();
				json_ptrs_stack.push(inner_value_ptr);
				current_map_ptr = inner_value_ptr->getMapPtr();
			}
			else {
				//top of the stack is json map
				(*current_map_ptr)[last_key] = std::make_shared<JsonMap>();
				inner_value_ptr = (*current_map_ptr)[last_key];
				json_ptrs_stack.push(inner_value_ptr);
				current_map_ptr = inner_value_ptr->getMapPtr();
			}
			break;
		}
		case CBRACKETS_CLOSE:
			if (prev_token.type == QUOTATION || prev_token.type == LITERAL) {
				(*current_map_ptr)[last_key] = last_value_ptr;
			}

			if (!json_ptrs_stack.empty()) {
				json_ptrs_stack.pop();
			}

			if (!json_ptrs_stack.empty()) {
				if (json_ptrs_stack.top()->type() == JSON_MAP) {
					current_map_ptr = json_ptrs_stack.top()->getMapPtr();
				}
			}
			else {
				// back at top level
				current_map_ptr = root->getMapPtr();
			}
			break;
		case SBRACKETS_OPEN:
			new_value_ptr = std::make_shared<JsonList>();
			if (!json_ptrs_stack.empty() && json_ptrs_stack.top()->type() == JSON_VECTOR) {
				list_ptr = json_ptrs_stack.top()->getListPtr();
				list_ptr->push_back(new_value_ptr);
				inner_value_ptr = list_ptr->back();
			}
			else {
				(*current_map_ptr)[last_key] = new_value_ptr;
				inner_value_ptr = (*current_map_ptr)[last_key];
			}
			json_ptrs_stack.push(inner_value_ptr);
			break;
		case SBRACKETS_CLOSE:
			if (prev_token.type == LITERAL || prev_token.type == QUOTATION) {
				list_ptr = json_ptrs_stack.top()->getListPtr();
				list_ptr->push_back(last_value_ptr);
			}
			if (!json_ptrs_stack.empty()) json_ptrs_stack.pop();
			break;
		case COMMA:
			if (prev_token.type == SBRACKETS_CLOSE || prev_token.type == CBRACKETS_CLOSE) break;
			if (json_ptrs_stack.top()->type() == JSON_VECTOR) {
				list_ptr = json_ptrs_stack.top()->getListPtr();
				list_ptr->push_back(last_value_ptr);
			}
			else{
				(*current_map_ptr)[last_key] = last_value_ptr;
			}
			break;
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
	return (*root->getMapPtr())[key];
}
const std::shared_ptr<JsonValue>& Json::operator[] (const std::string& key) const {
	auto& map = *root->getMapPtr();
	auto it = map.find(key);
	if (it == map.end()) throw std::out_of_range("Key not found in JSON object");
	return it->second;
}


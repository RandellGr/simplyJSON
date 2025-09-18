#pragma once
#include "Json.h"

bool is_token(char input) {
	if (
		input == '{' || input == '}' ||
		input == '[' || input == ']' ||
		input == '"' || input == ',' ||
		input == ':'
		) {
		return true;
	}
	else return false;
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
	file_content.erase(remove(file_content.begin(), file_content.end(), '\n'), file_content.end());
	file_content.erase(remove(file_content.begin(), file_content.end(), '\r'), file_content.end());

	tokens = tokenize(file_content);
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
			tokens.push_back({ QUOTATION, "" });
			break;
		case ':':
			tokens.push_back({ COLON, "" });
			break;
		case ' ':
			break;
		default:
			while (true) {
				working_buffer += json_string[i];
				if (is_token(json_string[i + 1])) break;
				i++;
			}
			tokens.push_back({ LITERAL, working_buffer });
			working_buffer.clear();
			break;
		}
	}
	return tokens; 
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
	bool					array_flag			= false;
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
			array_flag = true;
			break;
		case SBRACKETS_CLOSE:
			if (prev_token.type == LITERAL || prev_token.type == QUOTATION) {
				list_ptr = json_ptrs_stack.top()->getListPtr();
				list_ptr->push_back(last_value_ptr);
			}
			if (!json_ptrs_stack.empty()) json_ptrs_stack.pop();
			array_flag = false;
			break;
		case COMMA:
			if (prev_token.type == SBRACKETS_CLOSE) break;
			if (array_flag) {
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


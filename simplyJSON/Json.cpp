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
	size_t file_end;
	std::string file_content;
	std::vector<JsonToken> tokens;

	filestream.open(path, std::ios::in | std::ios::binary | std::ios::ate);
	if (!filestream.is_open()) throw std::runtime_error("could not open filestream at " + path + "\n");
	file_end = filestream.tellg();
	file_content.resize(file_end);
	filestream.read(file_content.data(), file_end);
	filestream.close();
	file_content.erase(remove(file_content.begin(), file_content.end(), '\n'), file_content.end());
	file_content.erase(remove(file_content.begin(), file_content.end(), '\r'), file_content.end());

	tokens = tokenize(file_content);
	parse(tokens);
}

Json::Json(const std::string& json_string) {
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

}
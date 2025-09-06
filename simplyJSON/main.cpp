#include "Common.h"
#include "Json.h"

int main() {
	std::fstream json_stream;
	Json test(json_stream, "test_json.json");
}
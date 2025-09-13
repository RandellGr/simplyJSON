#include "Common.h"
#include "Json.h"

int main() {
	std::fstream json_stream;
	Json test(json_stream, "test_json.json");
	std::cout << test.stringDump() << "\n\n\n post modification: \n";
	test["test_insert"] = std::make_shared<JsonString>("test_string");
	std::cout << test.stringDump();
	test.writeToFile(json_stream, "test_json.json");
}
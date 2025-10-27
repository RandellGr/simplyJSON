# Simple C++ 17 JSON Parser

A lightweight, header-based JSON parser and serializer written in modern C++ (C++17+).  
It supports parsing from strings and files, full JSON validation, custom error reporting, and object-style access to JSON data.

## Features
- Parse JSON from string or file  
- Serialize back to formatted JSON string or file  
- Full validation of structure and literals  
- Type-safe API (`JsonBool`, `JsonDouble`, `JsonString`, `JsonList`, `JsonMap`)  
- Smart pointer–based memory management  
- `makeJson()` factory template for automatic conversion to JSON types 
- Exception-based error handling via `ParseError`

## Include
Simply include these files into your project: Json.h, Template.h, Common.h, Json.cpp

## Example
- Creating Json from string and modifying values
```cpp
#include "Json.h"
using namespace smpj;

int main() {
    ParseError err;
    Json json("{\"name\":\"Alex\", \"age\":25, \"active\":true}", &err);

    if (err.get_id() != JSON_OK) {
        std::cerr << err.info() << std::endl;
        return 1;
    }

    std::cout << "Name: " << json["name"]->getString() << std::endl;
    std::cout << "Age: " << json["age"]->getDouble() << std::endl;

    // Modify values
    json["active"] = makeJson(false);

    // Serialize back to string
    std::cout << json.stringDump() << std::endl;

    // Write to file
    std::fstream file;
    json.writeToFile(f ile, "output.json");
}
```
- Creating JSON Programmatically
```cpp
using namespace smpj;

int main() {
    auto person = makeJson(std::unordered_map<std::string, JsonValuePtr>{
        {"name", makeJson("Alice")},
        {"age", makeJson(30)},
        {"skills", makeJson(std::vector<std::string>{"C++", "Python"})}
    });

    std::cout << person->asString() << std::endl;
}
```
- Opening a JSON file 
```cpp
int main() {
    // Error handler object
    ParseError err;

    // Create JSON object from a file
    std::fstream file;
    Json json(file, "data.json", &err);

    // Check for parsing or validation errors
    if (err.get_id() != JSON_OK) {
        std::cerr << "Failed to parse JSON file:\n"
                  << err.info() << std::endl;
        return 1;
    }

    // Optional: print the full formatted JSON
    std::cout << "Full JSON:\n" << json.stringDump() << std::endl;
}
```
# API overview

## Core Class
class Json
| Method                                                               | Description                                 |
| -------------------------------------------------------------------- | ------------------------------------------- |
| `Json()`                                                             | Create an empty JSON value                  |
| `Json(const std::string& str, ParseError* err)`                      | Parse JSON from string                      |
| `Json(std::fstream& file, const std::string& path, ParseError* err)` | Parse JSON from file                        |
| `stringDump()`                                                       | Serialize JSON object to a formatted string |
| `writeToFile(const std::string& path)`                               | Write JSON to a file                        |
| `operator[](const std::string&)`                                     | Access JSON object field by key             |
| `operator[](size_t)`                                                 | Access JSON array element by index          |

## Accessors
| Function      | Returns                                    |
| ------------- | ------------------------------------------ |
| `getString()` | `std::string`                              |
| `getDouble()` | `double`                                   |
| `getBool()`   | `bool`                                     |
| `getList()`   | `std::vector<JsonPtr>`                     |
| `getMap()`    | `std::unordered_map<std::string, JsonPtr>` |

## Helper

| Function          | Description                                    |
| ----------------- | ---------------------------------------------- |
| `makeJson(value)` | Create a JSON value from primitive or STL type |
| `ParseError`      | Holds parsing state and diagnostic info        |
| `err.get_id()`    | Returns `JsonParseErrors` enum                 |
| `err.info()`      | Returns human-readable error description       |



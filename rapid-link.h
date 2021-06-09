#pragma once
#include <string>
#include <tinyxml2.h>
#include <vector>

void FixLink(const std::string&, tinyxml2::XMLElement*);
std::string GetXPathFromUol(std::string, std::string);
std::string GetFilePathFromUol(std::string);

std::vector<tinyxml2::XMLElement*> remove_list;

std::string& rtrim(std::string& str) {
    if (const size_t endpos = str.find_last_not_of("\r"); endpos != std::string::npos) {
        str.substr(0, endpos + 1).swap(str);
    }
    return str;
}

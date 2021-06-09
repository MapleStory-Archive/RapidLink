#include "rapid-link.h"

#include <cstdint>
#include <iostream>
#include <regex>
#include <Windows.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/stacktrace.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/split.hpp>

#include "Tools/StopWatch.hpp"
#include "Tools/tixml2ex.h"

using namespace tinyxml2;
using std::cout, std::endl;
namespace fs = boost::filesystem;
namespace sw = stopwatch;

int main(int argc, char* argv[]) {
    fs::path target_dir("WZ");
    fs::recursive_directory_iterator it(target_dir), eod;
    sw::Stopwatch stopwatch;

    SetConsoleTitle(TEXT("RapidLink by WUT"));

    BOOST_FOREACH(fs::path const& path, std::make_pair(it, eod)) {
        if (is_regular_file(path)) {
            auto full_path = absolute(path).string();
            tinyxml2::XMLDocument xml_doc;

            if (const auto result = xml_doc.LoadFile(full_path.c_str()); result != XML_SUCCESS) continue;
            if (auto* root = xml_doc.FirstChildElement("imgdir"); root != nullptr) {
                FixLink(path.filename().string(), root);

                for (auto* remove : remove_list) {
                    remove->Parent()->DeleteChild(remove);
                }

                remove_list.clear();
                xml_doc.SaveFile(full_path.c_str());
                xml_doc.Clear();

                std::ifstream t(full_path);
                std::string str((std::istreambuf_iterator(t)), std::istreambuf_iterator<char>());
                std::ofstream out(full_path, std::ios::binary);

                out << rtrim(str);
                out.close();

                auto filename = path.filename().string();

                boost::erase_all(filename, "\"");
                cout << filename << " 완료" << endl;
            }
        }
    }

    remove_list.clear();

    uint64_t seconds = stopwatch.elapsed<sw::s>();
    cout << "경과 시간: " << seconds / 3600 << "시간" << seconds / 60 << "분" << seconds % 60 << "초" << endl;
    std::cin >> seconds;
}

void FixLink(const std::string& file_name, XMLElement* element) {
    for (auto* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        if (auto* root_type_name = child->Name(); root_type_name != nullptr) {
            if (!strcmp(root_type_name, "string")) {
                if (const auto* name = child->Attribute("name"); name != nullptr) {
                    if (!strcmp(name, "_inlink")) {
                        //인링크임
                        const auto xpath = GetXPathFromUol(file_name, child->Attribute("value"));
                        const auto* source_canvas = find_element(element->GetDocument()->RootElement(), xpath);
                        auto* target_element = dynamic_cast<XMLElement*>(child->Parent());

                        target_element->SetAttribute("width", source_canvas->Attribute("width"));
                        target_element->SetAttribute("height", source_canvas->Attribute("height"));
                        target_element->SetAttribute("basedata", source_canvas->Attribute("basedata"));
                        remove_list.emplace_back(child);
                    } else if (!strcmp(name, "_outlink") || !strcmp(name, "source")) {
                        //아웃링크, 소스임
                        tinyxml2::XMLDocument xml_doc;
                        const auto link = std::string(child->Attribute("value"));
                        const fs::path target_file_path(GetFilePathFromUol(link));
                        const auto target_file_name = target_file_path.filename();
                        const auto in_link = link.substr(link.find(".img/") + 5);

                        if (const auto result = xml_doc.LoadFile(target_file_path.string().c_str()); result == XML_SUCCESS) {
                            const auto xpath = GetXPathFromUol(target_file_name.string(), in_link);
                            const auto* source_canvas = find_element(xml_doc, xpath);
                            auto* target_element = dynamic_cast<XMLElement*>(child->Parent());

                            target_element->SetAttribute("width", source_canvas->Attribute("width"));
                            target_element->SetAttribute("height", source_canvas->Attribute("height"));
                            target_element->SetAttribute("basedata", source_canvas->Attribute("basedata"));
                            remove_list.emplace_back(child);
                        }
                    } else if (!strcmp(name, "_hash")) {
                        remove_list.emplace_back(child);
                    }
                }
            } else {
                FixLink(file_name, child);
            }
        }
    }
}

std::string GetXPathFromUol(const std::string file_name, std::string uol) {
    std::vector<std::string> splits, result;
    int32_t index = 0;

    split(splits, uol, boost::is_any_of("/"));
    result.emplace_back((boost::format("/imgdir[@name='%1%']/") % file_name).str());

    while (index < static_cast<int32_t>(splits.size())) {
        const auto format = index == static_cast<int32_t>(splits.size()) - 1
                                ? boost::format("canvas[@name='%1%']") % splits[index]
                                : boost::format("imgdir[@name='%1%']/") % splits[index];

        result.emplace_back(format.str());
        index++;
    }

    return boost::algorithm::join(result, "");
}

std::string GetFilePathFromUol(const std::string uol) {
    std::vector<std::string> splits, result;

    split_regex(splits, uol, boost::regex(".img"));

    auto path = (boost::format("WZ\\%1%.img.xml") % splits[0]).str();

    boost::replace_all(path, "/", "\\");
    result.emplace_back(fs::current_path().string());
    result.emplace_back(path);

    return boost::algorithm::join(result, "\\");
}

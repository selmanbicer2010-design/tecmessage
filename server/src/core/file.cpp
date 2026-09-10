#include "core/file.hpp"

#include <fstream>
#include <print>
#include <filesystem>

#define ERROR(log, erc) std::println("err; log: {}, erc: {}", (log), (erc));

namespace file{

    std::vector<uint8_t> readAsBinary(const std::string& filename)
    {
        if (!exists(filename)) { ERROR(filename + " not found", 404); return {}; }
        int v = 0;
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if ((v = !file.is_open())) { ERROR(filename + " could not be read", 100) return {}; }
        size_t fileSize = (size_t) file.tellg();
        std::vector<uint8_t> buffer(fileSize);
        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    std::string readAsString(const std::string& filename)
    {
        auto bytes = readAsBinary(filename);
        return std::string(bytes.begin(), bytes.end());
    }

    bool exists(const std::string& path)
    {
        return std::filesystem::exists(path);
    }

    long size(const std::string& filename)
    {
        if (!exists(filename)) { ERROR(filename + " not found", 404);; return -1; }
        return std::filesystem::file_size(filename);
    };

    namespace json
    {

        jarray::jarray(const jarray& other)
        { *this = other; }
        jarray::jarray(jarray&& other)
        { *this = std::move(other); }
        jarray& jarray::operator=(const jarray& other) {
            if (this == &other) return *this;
            clear();
            this->reserve(other.size());
            for (auto& value : other) {
                this->push_back(copyjval(*value));
            }
            return *this;
        }
        jarray& jarray::operator=(jarray&& other) {
            if (this == &other) return *this;
            clear();
            this->reserve(other.size());
            for (auto& value : other) {
                this->push_back(std::move(value));
            }
            return *this;
        }


        jobj::jobj(const jobj& other)
        { *this = other; }
        jobj::jobj(jobj&& other)
        { *this = std::move(other); }
        jobj& jobj::operator=(const jobj& other) {
            if (this == &other) return *this;
            clear();
            for (auto& [tag, value] : other) {
                this->insert({tag, copyjval(*value)});
            }
            return *this;
        }
        jobj& jobj::operator=(jobj&& other) {
            if (this == &other) return *this;
            clear();
            for (auto& [tag, value] : other) {
                this->insert({tag, std::move(value)});
            }
            return *this;
        }

        uptrjval makejarray(const std::initializer_list<jval>& initlist)
        {
            file::json::jarray arr;
            arr.reserve(initlist.size());
            for (auto& val : initlist)
            { arr.push_back(copyjval(val)); }
            return std::make_unique<file::json::jval>(arr);
        }

        uptrjval makejobj(const std::initializer_list<std::pair<std::string, jval>>& initlist)
        {
            file::json::jobj obj;
            for (auto& val : initlist)
            { obj.insert({val.first, copyjval(val.second)}); }
            return std::make_unique<file::json::jval>(obj);
        }

        tree deserialize(const std::string& path)
        {
            return parse(readAsString(path));
        }

        uptrjval parsedetail(std::string_view src)
        {
            // we expect no whitespace

            if (src == "null")
            {
                return makejprimitive(std::nullptr_t{nullptr});
            }

            bool st = (src == "true"); bool sf = (src == "false");
            if (st or sf)
            {
                bool val = st ? true : false;
                return makejprimitive(val);
            }

            bool firstnumeric = src[0] >= '0' and src[0] <= '9' or src[0] == '-';
            util::uresult<util::stodresult> result{false};
            if (firstnumeric) { result = util::stod(src); }
            if (firstnumeric and result)
            {
                // we HATE doubles around these parts
                double val = result.value;
                return makejprimitive(val);
            }

            bool quotewrapped = src[0] == '\"' and src[src.size() - 1] == '\"';
            if (quotewrapped)
            {
                std::string val = std::string{src.substr(1, src.size() - 2)};
                return makejprimitive(val);
            }

            bool curlywrapped = src[0] == '{' and src[src.size() - 1] == '}';
            if (curlywrapped)
            {
                jobj val;

                bool intagquotes = true;
                int32_t tagstart = 1;
                int32_t tagend = 1;

                bool inquotes = false;
                int32_t incurly = 0;
                int32_t inbrackets = 0;

                bool intag = true;

                for (int32_t i = 1; i < src.size() - 1; i++)
                {
                    if (intag)
                    {
                        if (src[i] == '\"')
                        {
                            intagquotes = false;
                        }
                        if (src[i] == ':' and !intagquotes)
                        {
                            intag = false;
                            tagend = i;
                        }
                        continue;
                    }

                    if (src[i] == '\"') { inquotes = !inquotes; }
                    if (src[i] == '[') { inbrackets++; }
                    if (src[i] == ']') { inbrackets--; }
                    if (src[i] == '{') { incurly++; }
                    if (src[i] == '}') { incurly--; }

                    if (src[i] == ',' and !inquotes and incurly == 0 and inbrackets == 0)
                    {
                        std::string tag = std::string{src.substr(tagstart + 1, tagend - tagstart - 2)};
                        val[tag] = parsedetail(src.substr(tagend + 1, i - tagend - 1));

                        tagstart = i + 1;
                        intag = true;
                        intagquotes = true;
                    }
                }
                if (!intag)
                {
                    std::string tag = std::string{src.substr(tagstart + 1, tagend - tagstart - 2)};
                    val[tag] = parsedetail(src.substr(tagend + 1, src.size() - 2 - tagend));
                }

                return wrapjval(std::move(val));
            }

            bool bracketwrapped = src[0] == '[' and src[src.size() - 1] == ']';
            if (bracketwrapped)
            {
                jarray val;

                bool inquotes = false;
                int32_t incurly = 0;
                int32_t inbrackets = 0;

                int32_t valstart = 1;
                for (int32_t i = 1; i < src.size() - 1; i++)
                {
                    if (src[i] == '\"') { inquotes = !inquotes; }
                    if (src[i] == '[') { inbrackets++; }
                    if (src[i] == ']') { inbrackets--; }
                    if (src[i] == '{') { incurly++; }
                    if (src[i] == '}') { incurly--; }
                    if (src[i] == ',' and !inquotes and incurly == 0 and inbrackets == 0)
                    {
                        val.push_back(parsedetail(src.substr(valstart, i - valstart)));
                        valstart = i + 1;
                    }
                }
                if (src.size() > 2)
                {
                    val.push_back(parsedetail(src.substr(valstart, src.size() - 1 - valstart)));
                }
                return wrapjval(std::move(val));;
            }

            return makejprimitive(std::nullptr_t{nullptr});
        }

        tree parse(const std::string& srcin)
        {
            tree out;
            auto src = srcin;
            bool inquotes = false;
            src.erase(std::remove_if(src.begin(), src.end(), [&](unsigned char x) {
                if (x == '\"')
                { inquotes = !inquotes; }
                return std::isspace(x) and !inquotes;
            }), src.end());
            out.root = std::move(*parsedetail(src));;
            return out;
        }

        std::string serializedetail(const jval& root)
        {
            std::string ret;

            root.visit<std::nullptr_t>([&](const std::nullptr_t&){
                ret = "null";
            });

            root.visit<bool>([&](bool value){
                ret = value ? "true" : "false";
            });

            root.visit<double>([&](double value){
                int32_t nearestint = std::round(value);
                if (std::abs(value - nearestint) < 0.0001) { ret = std::to_string(nearestint); }
                else { ret = util::tochars(value); }
            });

            root.visit<std::string>([&](const std::string& value){
                ret = "\"" + value + "\"";
            });

            root.visit<jarray>([&](const jarray& value){
                ret += "[";
                for (const uptrjval& arrval : value)
                {
                    if (!arrval) continue;
                    ret += serializedetail(*arrval) + ",";
                }
                if (ret.size() != 1) ret.pop_back();
                ret += "]";
            });

            root.visit<jobj>([&](const jobj& value){
                ret += "{";
                for (const auto& [tag, objval] : value)
                {
                    if (!objval) continue;
                    ret += "\"" + tag + "\":" + serializedetail(*objval) + ",";
                }
                if (ret.size() != 1) ret.pop_back();
                ret += "}";
            });

            return ret;
        }

        std::string serialize(const tree& tree)
        {
            return serializedetail(tree.root);
        }
    }
}

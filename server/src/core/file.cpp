#include "core/file.hpp"
#include "core/util.hpp"

#include <fstream>
#include <print>
#include <filesystem>
#include <source_location>
#include "core/enum.hpp"

namespace file{

    void ferr(std::string_view path, int32_t error_type, int32_t* err, std::source_location location) {
        if (err) {
            *err = error_type;
            ERROR(std::string{path} + util::this_function(location), error_type);
        }
    }

    std::vector<uint8_t> fread(std::string_view path, int32_t* err)
    {
        if (err) {*err = FILE_ERROR_NONE; }
        if (!exists(path, err)) {
            ferr(path, FILE_ERROR_NOT_FOUND, err);
            return {};
        }

        std::ifstream file(std::string{path}, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            ferr(path, FILE_ERROR_COULD_NOT_BE_READ, err);
            return {};
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<uint8_t> buffer(fileSize);
        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    bool exists(std::string_view path, int32_t* err)
    {
        if (err) {*err = FILE_ERROR_NONE; }
        std::error_code ec;
        bool exists = std::filesystem::exists(path, ec);
        if (ec) {
            ferr(path, FILE_ERROR_INVALID_ACCESS, err);
            return {};
        }
        if (!exists) {
            ferr(path, FILE_ERROR_NOT_FOUND, err);
            return {};
        }
        return exists;
    }

    uint64_t size(std::string_view path, int32_t* err)
    {
        if (err) {*err = FILE_ERROR_NONE; }
        if (!exists(path, err)) { if (err) {*err = FILE_ERROR_NOT_FOUND; } return 0; }
        std::error_code ec;
        uint64_t size = std::filesystem::file_size(path, ec);
        if (ec) {
            ferr(path, FILE_ERROR_INVALID_ACCESS, err);
            return {};
        }
        return size;
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
            auto buff = fread(path);
            return parse(std::string{buff.begin(), buff.end()});
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

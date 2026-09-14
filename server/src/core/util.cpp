#include "core/util.hpp"

#include <cmath>

namespace util{
    std::vector<std::string_view> strsplit(std::string_view str, const char splitter)
    {
        std::vector<std::string_view> out;
        size_t valstart = 0;
        for (size_t i = 0; i < str.size(); i++)
        {
            char c = str[i];
            if (c == splitter)
            {
                out.push_back(str.substr(valstart, i - valstart));
                valstart = i + 1;
                continue;
            }
        }
        out.push_back(str.substr(valstart, str.size() - valstart));
        return out;
    }

    uresult<stoi32result> stoi32(std::string_view str)
    {
        uresult<stoi32result> result{false};
        result.value = 0;
        result.charLength = 0;
        if (str.empty()) return result;
        bool negative = str[0] == '-';
        for (int i = 0 + negative; i < str.size(); i++)
        {
            char c = str[i];
            if (int num = c - '0'; num >= 0 and num <= 9)
            {
                result.success = true;
                result.value *= 10;
                result.value += num;
                result.charLength += 1;
            }
            else
            {
                break;
            }
        }
        result.charLength += negative;
        result.value *= !negative * 2 - 1;
        return result;
    }

    uresult<std::vector<uresult<stoi32result>>> stoi32vec(std::string_view str, const char splitter, uint32_t veclengthlimit)
    {
        if (veclengthlimit == 0) return false;
        std::vector<uresult<stoi32result>> out;
        out.reserve(veclengthlimit);
        auto split = strsplit(str, splitter);
        for (int i = 0; i < split.size(); i++)
        {
            if (i == veclengthlimit)
            {
                break;
            }
            if (auto result = util::stoi32(split[i]))
            {
                out.push_back(result);
            }
        }
        return {out.size() >= veclengthlimit, out};
    }

    uresult<stofresult> stof(std::string_view str)
    {
        uresult<stofresult> result{false};
        result.value = 0.f;
        result.charLength = 0;
        float decimalValue = 0.f;
        int decimalPlaces = 0;
        bool periodFound = false;
        if (str.empty()) return false;
        bool negative = str[0] == '-';
        for (int i = 0 + negative; i < str.size(); i++)
        {
            char c = str[i];
            if (c == '.' and periodFound)
            {
                break;
            }
            if (c == '.')
            {
                result.charLength++;
                periodFound = true;
            }
            else if (int num = c - 48; num >= 0 and num <= 9)
            {
                result.success = true;
                result.charLength++;
                decimalPlaces = periodFound ? ++decimalPlaces : decimalPlaces;
                float& write = periodFound ? decimalValue : result.value;
                write *= 10.f;
                write += num;
            }
            else
            {
                break;
            }
        }
        result.value += (decimalValue/(std::pow(10, decimalPlaces)));
        result.value *= !negative * 2 - 1;
        result.charLength += negative;
        return result;
    }

    uresult<std::vector<uresult<stofresult>>> stofvec(std::string_view str, const char splitter, uint32_t veclengthlimit)
    {
        if (veclengthlimit == 0) return false;
        std::vector<uresult<stofresult>> out;
        out.reserve(veclengthlimit);
        auto split = strsplit(str, splitter);
        for (int i = 0; i < split.size(); i++)
        {
            if (i == veclengthlimit)
            {
                break;
            }
            if (auto result = util::stof(split[i]))
            {
                out.push_back(result);
            }
        }
        return {out.size() >= veclengthlimit, out};
    }

    uresult<stodresult> stod(std::string_view str)
    {
        uresult<stodresult> result{false};
        result.value = 0.0;
        result.charLength = 0;
        double decimalValue = 0.0;
        int decimalPlaces = 0;
        bool periodFound = false;
        bool negative = str[0] == '-';
        for (int i = 0 + negative; i < str.size(); i++)
        {
            char c = str[i];
            if (c == '.' and periodFound)
            {
                break;
            }
            if (c == '.')
            {
                result.charLength++;
                periodFound = true;
            }
            else if (int num = c - 48; num >= 0 and num <= 9)
            {
                result.success = true;
                result.charLength++;
                decimalPlaces = periodFound ? ++decimalPlaces : decimalPlaces;
                double& write = periodFound ? decimalValue : result.value;
                write *= 10.0;
                write += num;
            }
            else
            {
                break;
            }
        }
        result.value += (decimalValue/(std::pow(10, decimalPlaces)));
        result.value *= !negative * 2 - 1;
        result.charLength += negative;
        return result;
    }

    uresult<std::vector<uresult<stodresult>>> stodvec(std::string_view str, const char splitter, uint32_t veclengthlimit)
    {
        if (veclengthlimit == 0) return false;
        std::vector<uresult<stodresult>> out;
        out.reserve(veclengthlimit);
        auto split = strsplit(str, splitter);
        for (int i = 0; i < split.size(); i++)
        {
            if (i == veclengthlimit)
            {
                break;
            }
            if (auto result = util::stod(split[i]))
            {
                out.push_back(result);
            }
        }
        return {out.size() >= veclengthlimit, out};
    }
}

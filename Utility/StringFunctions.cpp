#include "StringFunctions.h"

std::string GetHexRepresentation(const unsigned char* Bytes, size_t Length)
{
    std::ostringstream os;
    os.fill('0');
    os << std::hex;
    for (const unsigned char* ptr = Bytes; ptr < Bytes + Length; ++ptr) {
        os << std::setw(2) << (unsigned int)*ptr;
    }
    return os.str();
}

std::string lowercase(std::string in,bool stripOtherChars)
{
    std::string ret = "";
    for (int a = 0; a < in.length(); a++)
    {
        if (in[a] >= 'A' && in[a] <= 'Z')
            ret += tolower(in[a]);
        else if (in[a] >= 'a' && in[a] <= 'z')
            ret += in[a];
        else if (in[a] >= '0' && in[a] <= '9')
            ret += in[a];
        else if (in[a] == '-')
            ret += in[a];
        else if (in[a] == ' ')
            ret += in[a];
        else if (!stripOtherChars)
            ret += in[a];
    }
    return ret;
}

// trim whitespace from both ends
std::string& removeAllWhitespace(std::string& s)
{
    s.erase(std::remove_if(s.begin(),
        s.end(),
        [](unsigned char x) { return std::isspace(x); }),
        s.end());
    return s;
}

std::string leftTrim(std::string& s)
{
    for (int i = 0; i < s.length(); i++)
    {
        if (!std::isspace(s[i]))
        {
            return s.substr(i, s.length() - i);
        }
    }
    return "";
}

void replaceAll(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while (std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}
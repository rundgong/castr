#pragma once

#include <vector>
#include <string>

bool
found_in(const std::vector<std::string>& haystack,
         const std::string& needle);

bool
ends_with(const std::string& hayStack,
          const std::string& needle );

std::vector<std::string>
split(const std::string& str, char delim);

std::string ascii_to_lower(const std::string& str);

struct ExtensionMimeType
{
    std::string fileExtension;
    std::string mimeType;
};

// Returns mime type for known extensions or empty string
std::string
extension_to_mime_type(const std::string& filename);



// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#pragma once
#include <string>
#include <vector>

namespace Parfait {
namespace StringTools {
    std::vector<std::string> split(const std::string& s, const std::string& delimiter, bool skip_empty = true);
    std::vector<std::string> splitKeepDelimiter(const std::string& s, const std::string& delimiter);
    std::string join(const std::vector<std::string>& pieces, const std::string& delimiter);
    std::vector<std::string> joinAsVector(const std::vector<std::string>& pieces, const std::string& delimiter);
    std::string stripExtension(const std::string& s, const std::vector<std::string>& extensions);
    std::string stripExtension(std::string s, std::string extension);
    std::string stripExtension(std::string s);
    std::string stripFromBeginning(std::string s, std::string substring_to_remove);
    std::string getExtension(std::string s);
    std::string addExtension(std::string filename, std::string extension);

    std::string stripPath(std::string s);
    std::string getPath(std::string s);
    std::string lstrip(std::string s);
    std::string rstrip(std::string s);
    std::string strip(std::string s);

    std::string rpad(std::string s, int length, std::string fill = " ");
    std::string lpad(std::string s, int length, std::string fill = " ");

    std::vector<std::string> wrapLines(const std::string s, int column_width);

    std::string findAndReplace(std::string subject, const std::string& search, const std::string& replace);
    std::string tolower(const std::string& s);
    std::string toupper(std::string s);
    std::string removeCommentLines(std::string s, std::string comment_string);
    std::vector<std::string> removeWhitespaceOnlyEntries(const std::vector<std::string>& vec);
    std::string randomLetters(int n);
    bool contains(const std::string& s, const std::string& query);
    bool startsWith(const std::string& s, const std::string& query);
    bool endsWith(const std::string& s, const std::string& query);
    bool isInteger(const std::string& s);
    bool isDouble(std::string s);
    int toInt(const std::string& s);
    long toLong(const std::string& s);
    double toDouble(const std::string& s);
    std::vector<int> parseIntegerList(std::string s);

    std::vector<std::string> cleanupIntegerListString(std::string s);
    std::vector<std::string> parseQuotedArray(std::string s);
    void parseUgridFilename(std::string& filename,
                            std::string& ugrid_first_extension,
                            std::string& ugrid_last_extension);
    std::string combineUgridFilename(const std::string& filename,
                                     const std::string& ugrid_first_extension,
                                     const std::string& ugrid_last_extension);
}
}

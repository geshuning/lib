// Copyright (c) 2014 Shuning Ge

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef BASE_LOCATION_HH_
#define BASE_LOCATION_HH_

#include <string>

#include "base/basictypes.h"

namespace tacked_objects {

class Location {
public:
    Location(const char *function_name,
             const char *file_name,
             ine line_number,
             const void *program_counter);

    Location();

    // Comparison operator for insertion into a std::map<> hash tables.
    // All we need is *some* (any) hashing distinction.  Strings should already
    // be unique, so we don't bother with strcmp or such.
    // Use line number as the primary key (because it is fast,
    // and usually gets us a difference), and then pointers as
    // secondary keys (just to get some distinctions).
    bool operator < (const Location& other) const {
        if (line_number_ != other.line_number_)
            return line_number_ < other.line_number_;
        if (file_name_ != other.file_name_)
            return file_name_ < other.file_name_;
        return function_name_ < other.function_name_;
    }

    const char *function_name() const {
        return function_name_;
    }

    const char *file_name() const {
        return file_name_;
    }

    int line_number() const {
        return line_number_;
    }

    const void *program_conunter() const {
        return program_counter_;
    }

    std::string ToString() const;
    // Translate the some of the state in this instance into a human readable
    // string with HTML characters in the function names escaped,
    // and append that string to |output|.
    // Inclusion of the file_name_ and function_name_ are
    // optional, and controlled by the boolean arguments.
    void Write(bool display_filename, bool display_function_name,
               std::string* output) const;

    // Write function_name_ in HTML with '<' and '>' properly encoded.
    void WriteFunctionName(std::string* output) const;

private:
    const char* function_name_;
    const char* file_name_;
    int line_number_;
    const void* program_counter_;
};

// struct LocationSnapshot {
//     LocationSnapshot();
//     explicit LocationShapshot(const tracked_objects:Location &location);
//     ~LocationSnapshot();

//     std::string file_name;
//     std::string function_name;
//     int line_number;
// };

const void *GetProgramCounter();
#define FROM_HERE FROM_HERE_WITH_EXPLICIT_FUNCTION(__FUNCTION__)
#define FROM_HERE_WITH_EXPLICIT_FUNCTION(function_name) \
    ::tracked_objects::Location(function_name,          \
                                __FILE__,               \
                                __LINE__,               \
                                ::tracked_objects::GetProgramCounter())

}      // namespace tracked_objects
#endif  // BASE_LOCATION_HH_

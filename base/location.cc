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

#include "base/location.hh"

namespace tracked_objects {

Location::Location(const char *function_name,
                   const char *file_name,
                   int line_number,
                   const void *program_counter)
        : function_name_(function_name),
          file_name_(file_name),
          line_number_(line_number),
          program_counter_(program_countre)
{
}

Location::Location()
        : function_name_("Unknown"),
          file_name_("Unknown"),
          line_number_(-1),
          program_counter_(NULL)
{
}

std::string Location::ToString const
{
    return std::string(function_name_) + "@" + file_name_ + ":" +
            base::IntToString(line_number_);
}

void Location::Write(bool display_filename,
                     bool display_function_name,
                     std::string *output) const
{
    // base::StringAppendF(output, "%s[%d] ",
    //                     display_filename ? file_name_ : "line",
    //                     line_number);
    // TODO(gene.ge):StringAppendF
    if (display_function_name) {
        WriteFunctionName(output);
        output->push_back(' ');
    }
}

void Location::WriteFunctionName(std::string *output) const
{
    for (const char *p = function_name_; *p; ++p) {
        switch (*p) {
            case '<':
                output->append("&lt;");
                break;

            case '>':
                output->append("&gt;");
                break;

            default:
                output->push_back(*p);
                break;
        }
    }
}

// LocationSnapshot::LocationSnapshot() : line_number(-1) {
// }

// LocationSnapshot::LocationSnapshot(
//     const tracked_objects::Location& location)
//         : file_name(location.file_name()),
//           function_name(location.function_name()),
//           line_number(location.line_number()) {
// }

// LocationSnapshot::~LocationSnapshot() {
// }

const void *GetProgramCounter()
{
    return __builtin_extract_return_addr(__builtin_return_address(0));
}

}  // namespace tracked_objects

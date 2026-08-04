# python file for the builtin samfile

input_path = "buildin.samfile"
output_path = "libs/sfp/include/sfp/generated.hpp"

with open(input_path, "r", encoding="utf-8") as f:
    lines = f.readlines()

# remove comments starting with # and empty lines
filtered_lines = [
    line.rstrip()
    for line in lines
    if not line.lstrip().startswith("#") and not line.lstrip().startswith("//") and not line.lstrip().startswith("--")and line.strip()
]

filtered = "\n".join(filtered_lines)

with open(output_path, "w", encoding="utf-8") as f:
    f.write("""// THIS FILE IS AUTO GENERATED!
// DO NOT EDIT IT!!!

#include <string>

inline std::string buildin_samfile_content = R"(
""")

    f.write(filtered)

    f.write("""
)";
""")

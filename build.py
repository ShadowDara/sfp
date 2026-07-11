# python file for the buildin samfile

with open("include/sfp/generated.hpp", "w", encoding="utf-8") as f:
    f.write("""// THIS FILE IS AUTO GENERATED!
// DO NOT EDIT IT!!!

#include <string>

inline std::string buildin_samfile_content = R"(
""")

    with open("buildin.samfile", "r", encoding="utf-8") as f2:
        f.write(f2.read())
    f.write(")\";\n")

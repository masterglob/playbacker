#ifndef I_pbkr_types_h_I
#define I_pbkr_types_h_I

#include <string>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <fstream>

namespace PBKR
{
using namespace std;

typedef vector<string,allocator<string>> StringVect;

// Add static inline functions just because eclipse cannot parse some functions or templates
// and shows error where there aren't
static inline bool string_starts_with (const std::string& s, const std::string& prefix)
{
    return s.rfind(prefix, 0) == 0;
}
static inline int file_open_write (const char* filename)
{
    return open (filename, O_WRONLY);
}

template<typename... Args> static inline int err_printf (const char * fmt,Args... args)
{
    return fprintf(stderr,fmt, args...);
}

static inline std::string substring(const std::string& s, size_t from, size_t len = std::string::npos)
{
    return s.substr(from, len);
}

static inline std::string strerase(std::string& s, size_t from, size_t len = std::string::npos)
{
    return s.erase(from, len);
}

} // namespace PBKR

static inline void fread(std::ifstream& f, char* buffer, size_t count)
{
    f.read (buffer, count);
}
#define FOR(it, obj) for (auto it(obj.begin()); it != obj.end(); it ++)

static inline bool starts_with(const std::string& str, const std::string& prefix)
{
    return (prefix == str.substr(0,prefix.length()));
}
#endif // I_pbkr_types_h_I

#ifndef I_pbkr_types_h_I
#define I_pbkr_types_h_I

#include <string>
#include <vector>
#include <fcntl.h>

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

} // namespace PBKR

#endif // I_pbkr_types_h_I

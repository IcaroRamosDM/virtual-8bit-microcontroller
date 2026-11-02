#pragma once

#include <cstddef>
#include <string>

namespace vm8
{
// Replace a file only after its complete replacement has been written and closed.
bool write_file_atomic(const std::string &path, const void *data, std::size_t size);
}

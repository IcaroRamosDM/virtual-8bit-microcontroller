#include "file_io.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#endif

namespace vm8
{
namespace
{
namespace fs = std::filesystem;

struct TemporaryFile
{
  fs::path path;

  ~TemporaryFile()
  {
    std::error_code ignored;
    fs::remove(path, ignored);
  }
};

FILE *create_exclusive(const fs::path &path)
{
#ifdef _WIN32
  const int descriptor =
      _wopen(path.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
  const int descriptor = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
#endif
  if (descriptor < 0)
  {
    return nullptr;
  }

#ifdef _WIN32
  FILE *stream = _fdopen(descriptor, "wb");
  if (!stream)
  {
    _close(descriptor);
  }
#else
  FILE *stream = fdopen(descriptor, "wb");
  if (!stream)
  {
    close(descriptor);
  }
#endif
  if (!stream)
  {
    std::error_code ignored;
    fs::remove(path, ignored);
  }
  return stream;
}
}

bool write_file_atomic(const std::string &path, const void *data, std::size_t size)
{
  if (path.empty() || (data == nullptr && size != 0))
  {
    return false;
  }
  std::error_code error;
  fs::path target = fs::u8path(path);
  const bool exists = fs::exists(target, error);
  if (error)
  {
    return false;
  }
  if (exists)
  {
    // Follow an existing symbolic link, retaining the link itself on replacement.
    target = fs::canonical(target, error);
    if (error || !fs::is_regular_file(target, error) || error)
    {
      return false;
    }
  }

  const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
  constexpr unsigned MAX_ATTEMPTS = 100;
  for (unsigned attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
  {
    const fs::path temporary_path = target.parent_path() / (".vm8-save-" + std::to_string(nonce) +
                                                            "-" + std::to_string(attempt) + ".tmp");
    FILE *stream = create_exclusive(temporary_path);
    if (!stream)
    {
      if (errno == EEXIST)
      {
        continue;
      }
      return false;
    }
    const TemporaryFile temporary{temporary_path};
    const bool written = size == 0 || std::fwrite(data, 1, size, stream) == size;
    const bool closed = std::fclose(stream) == 0;
    if (!written || !closed)
    {
      return false;
    }
    if (exists)
    {
      const auto permissions = fs::status(target, error).permissions();
      if (error)
      {
        return false;
      }
      fs::permissions(temporary.path, permissions, error);
      if (error)
      {
        return false;
      }
    }
    fs::rename(temporary.path, target, error);
    return !error;
  }
  return false;
}
}

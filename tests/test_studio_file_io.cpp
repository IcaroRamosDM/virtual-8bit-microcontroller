#include "file_io.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

#ifndef _WIN32
#include <csignal>
#include <sys/resource.h>
#endif

namespace fs = std::filesystem;

static std::string read_file(const fs::path &path)
{
  std::ifstream stream(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

int main()
{
  // This dedicated directory is created fresh; never remove an existing user's path.
  const fs::path directory = "build/test-studio-file-io";
  const bool created = fs::create_directory(directory);
  assert(created);
  const fs::path file = directory / fs::u8path("programa com acentua\u00e7\u00e3o.asm");
  const std::string original = "HALT\n";
  bool saved = vm8::write_file_atomic(file.u8string(), original.data(), original.size());
  assert(saved && read_file(file) == original);
  const std::string replacement = "LDI A, 42\nHALT\n";
  saved = vm8::write_file_atomic(file.u8string(), replacement.data(), replacement.size());
  assert(saved && read_file(file) == replacement);

  saved = vm8::write_file_atomic(directory.u8string(), original.data(), original.size());
  assert(!saved && fs::is_directory(directory));
  saved = vm8::write_file_atomic((directory / "missing/file.asm").u8string(),
                                 original.data(),
                                 original.size());
  assert(!saved && read_file(file) == replacement);

#ifndef _WIN32
  // A real write failure exercises the stdio flush/close path without filling the disk.
  struct rlimit original_limit
  {
  };
  const int read_limit = getrlimit(RLIMIT_FSIZE, &original_limit);
  assert(read_limit == 0);
  struct rlimit limited = original_limit;
  limited.rlim_cur = 1024;
  const auto previous_signal = std::signal(SIGXFSZ, SIG_IGN);
  const int set_limit = setrlimit(RLIMIT_FSIZE, &limited);
  assert(set_limit == 0);
  const std::string oversized(4096, 'A');
  saved = vm8::write_file_atomic(file.u8string(), oversized.data(), oversized.size());
  const int restored = setrlimit(RLIMIT_FSIZE, &original_limit);
  std::signal(SIGXFSZ, previous_signal);
  assert(restored == 0);
  assert(!saved && read_file(file) == replacement);

  const fs::path link = directory / "linked.asm";
  fs::create_symlink(file.filename(), link);
  saved = vm8::write_file_atomic(link.u8string(), original.data(), original.size());
  assert(saved && fs::is_symlink(link) && read_file(file) == original);
  fs::remove(link);
#endif

  saved = vm8::write_file_atomic(file.u8string(), nullptr, 0);
  assert(saved && read_file(file).empty());
  std::size_t remaining = 0;
  for (const auto &entry : fs::directory_iterator(directory))
  {
    assert(entry.path() == file);
    ++remaining;
  }
  assert(remaining == 1);
  fs::remove(file);
  fs::remove(directory);
  std::cout << "Studio file saving: replacement, UTF-8 paths, failures and cleanup passed.\n";
}

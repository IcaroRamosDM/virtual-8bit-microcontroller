#pragma once

#include <stdio.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <windows.h>
#endif

/* Tests run from the repository root, where build/ is writable. */
static inline FILE *test_tmpfile(void)
{
#ifdef _WIN32
  /* Legacy MSVCRT tmpfile() can require write access to the drive root. */
  enum { TEST_STREAM_PATH_CAPACITY = 128 };
  static unsigned long sequence = 0;
  char path[TEST_STREAM_PATH_CAPACITY];
  const int path_length = snprintf(
    path,
    sizeof path,
    "build/test_stream_%lu_%lu.tmp",
    (unsigned long)GetCurrentProcessId(),
    sequence++
  );

  if (path_length < 0 || (size_t)path_length >= sizeof path)
  {
    return NULL;
  }

  /* CREATE_NEW refuses to overwrite an existing file. */
  HANDLE const handle = CreateFileA(
    path,
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    CREATE_NEW,
    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
    NULL
  );

  if (handle == INVALID_HANDLE_VALUE)
  {
    return NULL;
  }

  const int descriptor = _open_osfhandle(
    (intptr_t)handle,
    _O_RDWR | _O_BINARY
  );

  if (descriptor == -1)
  {
    (void)CloseHandle(handle);
    return NULL;
  }

  FILE *const stream = _fdopen(descriptor, "w+b");

  if (stream == NULL)
  {
    (void)_close(descriptor);
  }

  /* fclose() owns the descriptor, handle, and delete-on-close cleanup. */
  return stream;
#else
  return tmpfile();
#endif
}

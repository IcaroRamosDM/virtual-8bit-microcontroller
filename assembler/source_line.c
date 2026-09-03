#include "source_line.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

void source_line_normalize(char *line)
{
  char *comment_start = strchr(line, ';');

  if (comment_start != NULL)
  {
    *comment_start = '\0';
  }

  char *content_start = line;

  while (isspace((unsigned char)*content_start))
  {
    ++content_start;
  }

  size_t content_length = strlen(content_start);

  while (
      (content_length > 0) &&
      isspace((unsigned char) content_start[content_length - 1])
  )
  {
    --content_length;
  }

  content_start[content_length] = '\0';

  if (content_start != line)
  {
    memmove(
        line,
        content_start,
        content_length + 1
    );
  }
}

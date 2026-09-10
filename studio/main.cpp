#include "app.h"
#include <FL/Fl.H>
#include <cstring>

int main(int argc, char **argv)
{
  if (argc == 2 && std::strcmp(argv[1], "--self-test") == 0) return vm8::self_test();
  vm8::configure_theme();
  vm8::App app;
  if (argc == 3 && std::strcmp(argv[1], "--ui-smoke-test") == 0) return app.smoke_test(argv[2]);
  app.show();
  return Fl::run();
}

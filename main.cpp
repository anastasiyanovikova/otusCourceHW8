#include "options.h"

int main(int argc, const char *argv[])
{
  options app(argc, argv);
  return app.exec();
}
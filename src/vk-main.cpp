#include <stdio.h>

#include "vk-def.h"

int main (void)
{
  puts ("Hello World!");
  puts ("This is " PACKAGE_STRING ".");

  TestMain prog;
  VkResult res;
  res = prog.init();

  printf("Result is %d\n", res);

  return 0;
}

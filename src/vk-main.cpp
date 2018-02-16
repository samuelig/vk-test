#include <stdio.h>

#include "vk-def.h"

int main (void)
{
  TestMain prog;
  VkResult res;
  res = prog.init();

  printf("\n\n\n------------------------------------- \n");
  printf("\t Hello World!\n");
  printf("\t This is %s.\n", PACKAGE_STRING);
  printf("\t Result is %d\n", res);
  printf("------------------------------------- \n\n\n");

  prog.run();

  prog.cleanup();

  return 0;
}

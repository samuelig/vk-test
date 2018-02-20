#include <stdio.h>

#include "vk-test.h"

int main (void)
{
  VulkanTest prog;

  prog.init();

  printf("\n\n\n------------------------------------- \n");
  printf("\t Hello World!\n");
  printf("\t This is %s.\n", PACKAGE_STRING);
  printf("------------------------------------- \n\n\n");

  prog.run();

  prog.cleanup();

  return 0;
}

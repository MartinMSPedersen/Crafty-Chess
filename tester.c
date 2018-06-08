#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[])
{
  FILE *input, *output;
  char buffer[512], *bytes;
  float check, onerep, recapture, mate;

  input = fopen(argv[1], "r");
  output = fopen(argv[2], "w+");
  bytes = (char *) 1;
  while (!feof(input)) {
    fscanf(input, "%f %f %f %f", &check, &onerep, &recapture, &mate);
    fprintf(output,
        "crafty <wac.run ext/check=%4.2f ext/onerep=%4.2f ext/recap=%4.2f ext/mate=%4.2f > waclog-c%4.2f-o%4.2f-r%4.2f-m%4.2f\n",
        check, onerep, recapture, mate, check, onerep, recapture, mate);
  }
  fclose(input);
  fclose(output);
}

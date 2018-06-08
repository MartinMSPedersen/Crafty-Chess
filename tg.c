#include <stdio.h>
int main() {
  int g;
  while (1) {
    printf("grade: ");
    scanf("%d", &g);
    printf("ug=%d  g=%d\n", (100 * (g-10)) / 70, (100 * (g-10)) / 80);
  }
}

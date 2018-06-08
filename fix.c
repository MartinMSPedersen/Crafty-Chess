#include <stdio.h>
#include <string.h>
main() {
  char line[256], *t, *t1, *t2, *bm, *title;
  int i,j=0;
  for (i=0;i<14;i++) {
    j++;
    fgets(line,256,stdin);
    bm=strstr(line,"bm ")+3;
    *(strstr(line,"bm")-1)=0;
    t=strchr(bm,';');
    *t=0;
/*
    t1=strchr(t+1,'"');
    t2=strchr(t1+1,'"');
    *(t2+1)=0;
*/
    printf("title gs2930 position %d\n",j);
    printf("setboard %s\n",line);
    printf("solution %s\n",bm);
  }
}

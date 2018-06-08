#include <string.h>
void extract(int n, char in[], char out[]) {
  char save[16];
  int c, next=0;
  for (c=0;c<n;c++)
    save[next++]=in[c];
  for (c=n+1;c<strlen(in);c++)
    save[next++]=in[c];
  save[next]=0;
  strcpy(out,save);
}
void nextc(char word[], char left[]) {
  int i,j;
  char in[16], out[16];
  if (strlen(left) > 0) {
    for (i=0;i<strlen(left);i++) {
      strcpy(in,word);
      extract(i,left,out);
      j=strlen(in);
      in[j]=left[i];
      in[j+1]=0;
      nextc(in,out);
    }
  }
  else printf("%s\n",word);
}
void next1(char word[], char left[]) {
  int i,j;
  char in[16], out[16];
  if (strlen(left) > 0) {
    for (i=0;i<strlen(left);i++) {
      strcpy(in,word);
      extract(i,left,out);
      j=strlen(in);
      in[j]=left[i];
      in[j+1]=0;
      nextc(in,out);
      in[0]=toupper(in[0]);
      nextc(in,out);
    }
  }
  else printf("%s\n",word);
}
main(int argc, char *argv[]) {
  int cnt=strlen(argv[1]);
  int n;
  char word[16];
  word[0]=0;
  next1(word,argv[1]);
}

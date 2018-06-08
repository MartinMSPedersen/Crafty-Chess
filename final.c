int main() {
  int g1=1, g2=1, g3=1;

  while (g1+g2+g3) {
    printf("g1 g2 g3? ");
    scanf("%d %d %d",&g1,&g2,&g3);
    g1=g1*100/30;
    g2=g2*100/32;
    g3=g3*100/20;
    printf("t1=%d  t2=%d  lab=%d  g1=%d  g2=%d  g3=%d\n",
      g1, g2, g3, (int) (0.50*g1+0.30*g2+0.20*g3+.5),
                  (int) (0.40*g1+0.40*g2+0.20*g3+.5),
                  (int) (0.30*g1+0.50*g2+0.20*g3+.5));
  }
}

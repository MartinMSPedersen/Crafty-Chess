main() {
  int g1, g2, f;
  while (1) {
    printf("g1, g2: ");
    scanf("%d %d",&g1, &g2);
    g1=g1*100/95;
    g2=g2*100/90;
    printf("low, avg, high = %d  %d  %d\n", (int) (g1*.4+g2*.6),
                                            (int) (g1*.5+g2*.5),
                                            (int) (g1*.6+g2*.4));
  }
}

main() {
  int g1, g2, g3;
  float final1, final2, final3;
  while (g1) {
    printf("g1-3?: ");
    scanf("%d %d %d",&g1,&g2,&g3);
    final1=(float)g1/.29*.4 + (float) g2/.28*.4 + (float) g3*.2;
    final2=(float)g1/.29*.5 + (float) g2/.28*.3 + (float) g3*.2;
    final3=(float)g1/.29*.3 + (float) g2/.28*.5 + (float) g3*.2;
    printf("final grade span=%6.2f  %6.2f  %6.2f\n",final1,final2,final3);
  }
}

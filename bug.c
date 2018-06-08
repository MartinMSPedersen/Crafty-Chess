#include <stdio.h>

enum SQUARES { A8,B8,C8,D8,E8,F8,G8,H8,
               A7,B7,C7,D7,E7,F7,G7,H7,
               A6,B6,C6,D6,E6,F6,G6,H6,
               A5,B5,C5,D5,E5,F5,G5,H5,
               A4,B4,C4,D4,E4,F4,G4,H4,
               A3,B3,C3,D3,E3,F3,G3,H3,
               A2,B2,C2,D2,E2,F2,G2,H2,
               A1,B1,C1,D1,E1,F1,G1,H1,
               NO_SQUARE };

typedef unsigned long long Bitmap;
Bitmap bm_mask[64];
Bitmap to_boundary[64];

void DrawBitmap(Bitmap bmap) {
        int c;
        for (c=0;c<64;c++) {
                if (bmap&bm_mask[c]) printf("1"); else printf("0");
                if ((c%8)==7) printf("\n");
        }
        printf("\n");
}

void Initialize() {

        int c, t;
        Bitmap b=1;
        for (c=0;c<64;c++)
                bm_mask[c]=(b<<c);


        for (c=0;c<64;c++) {
                to_boundary[c]=0;
                for (t=1;t<8;t++) {
                        if (c==E4) printf("considering: %d\n",t);
                        if (c-(t*8)<0) break;
                        to_boundary[c]|=bm_mask[c-(t*8)];
                }
        }

        DrawBitmap(to_boundary[E4]);

}

int main(int argc,char *argv[]) {
        Initialize();
        return 0;
}


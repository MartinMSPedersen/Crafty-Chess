#if defined(FUTILITY)
      else {
         if (abs(alpha) < (MATE-500) && ply > 4 && !tree->in_check[ply]) {
            if (wtm) {
               if (depth<3*INCPLY && (Material+FUTILITY_MARGIN)<=alpha) {
                  fprune=1;
               }
               else if (depth>=3*INCPLY && depth<5*INCPLY &&
                  (Material+RAZOR_MARGIN)<=alpha)
                  extended-=60;
            }
            else {
               if (depth<3*INCPLY && (-Material+FUTILITY_MARGIN)<=alpha) {
                  fprune=1;
               }
               else if (depth>=3*INCPLY && depth<5*INCPLY &&
                  (-Material+RAZOR_MARGIN)<=alpha)
                  extended-=60;
            }
         }
      }
#endif

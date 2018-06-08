int Quiesce(int board[64], int alpha, int beta, int wtm, int ply) 
{
	move list[256];
	int e, c, f, length;
	c=Check(board, wtm);
	if (c) length=genmvs(board, list, wtm);
	else {
    		e=eval(board, wtm);
		if (e >= beta) return(beta);
		if (e > alpha) alpha=e;
		length=gencaps(board, list, wtm);
	}
    	f=0;
	while (length--) {
		makemove(list[length], board);
		if (!Check(board, wtm)) {
    			f=1;
			e=-Quiesce(board, -beta, -alpha, -wtm, ply+1);
			if (e>alpha) {
				if (e >= beta) {
					unmakemove(list[length], board);
					return(beta);
				}
				alpha=e;
			}
		}
		unmakemove(list[length], board);
	}
	if ((!f) && (c)) return(ply-10000);
   	return(alpha);
}

int AlphaBeta(int board[64], int depth, int maxply, int wtm, int alpha, int
beta, int time_remaining)
{
	move moves[256];
	int i, score, in_check, lm;
	clock_t start, current;
	start=time(NULL);
	if (!time_remaining) {
		complete[maxply]=0;
		return(score);
	}
	lm=0;
	in_check=Check(board, wtm);
	if (in_check) {
		depth++;
		maxply++;
	}
	if (depth == 0) {
		searched++;
		clear(captured);
		return Quiesce(board, alpha, beta, wtm, 0);
	}
	else {
		i=genmvs(board, moves, wtm);
		while (i--) {
			lm=1;
			makemove(moves[i], board);
			if (!Check(board, wtm)) {
				current=time(NULL);
				score=-AlphaBeta(board, depth-1, maxply, -wtm, -beta, -alpha,
time_remaining-difftime(current, start));
				if (score > alpha) {
					if (score >= beta) {
						return(beta);
						unmakemove(moves[i], board);
					}
					if (depth == maxply) mv=i;
					alpha=score;
				}
			}
			unmakemove(moves[i], board);
		}
		if (!lm) {
			if (in_check) return(depth-INFINITY);
			return(0);
		}
		return(alpha);
	}
}


#include <stdio.h>
#include <stdlib.h>

static int
PuTTY_256(int r, int g, int b)
{
    int best_diff = -1, nearest_static = 0;
    int nr, ng, nb;

    /*
     * Annoyingly the 6x6x6 cube that XTerm uses by default (and so our
     * cube) isn't the websafe colours. This means the standard method
     * of calculating the best match won't work, but I can do better
     * than xterm does because we don't ever change the mapping.
     */

    nr = (r-36)/40; if (nr == 0) nr = (r+47)/95;
    ng = (g-36)/40; if (ng == 0) ng = (g+47)/95;
    nb = (b-36)/40; if (nb == 0) nb = (b+47)/95;

    nearest_static = 16 + nb + ng * 6 + nr * 36;

    if ( (abs(r-g)<20 && abs(g-b)<20 && abs(b-r)<20 ) || (nr==ng && ng==nb))
    { /* If it's grey or it becomes one. */
	int tc, tg;
	int tw, nw;
	tw = (r+g+b)/3; nw = 232 + (tw-4)/10; if (nw > 255) nw = 255;

	tc = nr ? nr * 40 + 55 : 0;
	tc = ng ? ng * 40 + 55 : 0;
	tc = nb ? nb * 40 + 55 : 0;
	tc /= 3;
	tg = (nw-232) * 10 + 8;

	if ( (tc-tw)*(tc-tw) >= (tg-tw)*(tg-tw) )
	    nearest_static = nw;
    }

    return nearest_static;
}

static int
XTerm_256(int r, int g, int b)
{
    int best_diff = -1;
    int nearest_static = 0;
    int c;
    for(c=16; c<256; c++) {
        int i = c-16;
	int d, this_diff = 0;
        int nr, ng, nb;
        if (c<232) {
            nr = i / 36; ng = (i / 6) % 6; nb = i % 6;
            nr = nr ? nr * 40 + 55 : 0;
            ng = ng ? ng * 40 + 55 : 0;
            nb = nb ? nb * 40 + 55 : 0;
        } else {
            i = i - 216;
            nr=ng=nb = i * 10 + 8;
        }

	d = nr-r; this_diff += d*d;
	d = ng-g; this_diff += d*d;
	d = nb-b; this_diff += d*d;

        if (best_diff<0 || best_diff>this_diff) {
            nearest_static = c;
            best_diff = this_diff;
        }
    }

    return nearest_static;
}

static int
XTerm_256_unused(int r, int g, int b)
{
    double best_diff = -1;
    int nearest_static = 0;
    int c;
    for(c=16; c<256; c++) {
        int i = c-16;
	double d, this_diff = 0;
        int nr, ng, nb;
        if (c<232) {
            nr = i / 36; ng = (i / 6) % 6; nb = i % 6;
            nr = nr ? nr * 40 + 55 : 0;
            ng = ng ? ng * 40 + 55 : 0;
            nb = nb ? nb * 40 + 55 : 0;
        } else {
            i = i - 216;
            nr=ng=nb = i * 10 + 8;
        }

	d = nr-r; d *= 0.30; this_diff += d*d;
	d = ng-g; d *= 0.61; this_diff += d*d;
	d = nb-b; d *= 0.11; this_diff += d*d;

        if (best_diff<0 || best_diff>this_diff) {
            nearest_static = c;
            best_diff = this_diff;
        }
    }

    return nearest_static;
}


int
main(int argc, char ** argv)
{
    int r=0, g=0, b=0, p=0, x=0, v=0;

    if (argc>2) {
	r=g=b=atoi(argv[1]);
	if (argc>2) g=atoi(argv[2]);
	if (argc>3) b=atoi(argv[3]);

	p = PuTTY_256(r,g,b);
	x = XTerm_256(r,g,b);

	printf("(%d,%d,%d) -> p(%d) x(%d)\n", r,g,b, p, x);
	printf("\033[48;5;%dm p \033[m", p);
	printf("\033[48;2;%d;%d;%dm r \033[m", r,g,b);
	printf("\033[48;5;%dm x \033[m", x);
	printf("\n");
	exit(0);
    }

    if (argc>1) v=atoi(argv[1]);

    for(r=0; r<256; r+=16)
	for(g=0; g<256; g+=16)
	    for(b=0; b<256; b+=16)
	    {
		if (v<2) {
		    if(!v) p = PuTTY_256(r,g,b); else p = XTerm_256(r,g,b);
		    printf("\033[48;5;%dm \033[m", p);
		} else
		    printf("\033[48;2;%d;%d;%dm \033[m", r,g,b);
		x++;
		if (x%128 == 0)
		    printf("\n");
	    }

    for(g=0; g<256; g+=2)
    {
	if (v<2) {
	    if(!v) p = PuTTY_256(g,g,g); else p = XTerm_256(g,g,g);
	    printf("\033[48;5;%dm \033[m", p);
	} else
	    printf("\033[48;2;%d;%d;%dm \033[m", g,g,g);
    }
    printf("\n");
}

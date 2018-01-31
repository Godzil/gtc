typedef struct {
	int sel,scr,h,n;
} SCROLL;

#define ScrollInit(s) ((s)->sel=0,(s)->scr=0)
#define ScrollChg(s,n) ((s)->sel+=n)
#define ScrollUp(s) (--(s)->sel)
#define ScrollDn(s) (++(s)->sel)
#define ScrollHasUpArrow(s) (!!((s)->scr))
#define ScrollHasDnArrow(s) ((s)->scr+(s)->h<(s)->n)
void ScrollUpd(SCROLL *s) {
	int n=s->n;
	while (s->sel<0) s->sel+=n;
	while (s->sel>=n) s->sel-=n;
	/* Now s->sel is OK, let's update s->scr */
	if (s->sel<s->scr) s->scr=s->sel;
	if (s->sel>=s->scr+s->h) s->scr=s->sel-s->h+1;
}
void ScrollUpdNoWrap(SCROLL *s) {				/* same as ScrollUpd, but doesn't wrap */
	int n=s->n;
	if (s->sel<0) s->sel=0;
	if (s->sel>=n) s->sel=n-1;
	/* Now s->sel is OK, let's update s->scr */
	if (s->sel<s->scr) s->scr=s->sel;
	if (s->sel>=s->scr+s->h) s->scr=s->sel-s->h+1;
}
void ScrollUpdNoWrapDots(SCROLL *s) {		/* same as ScrollUpdNoWrap, but leaves 1 blank on the border */
	int n=s->n;
	if (s->sel>=n) s->sel=n-1;
	if (s->sel<0) s->sel=0;
	/* Now s->sel is OK, let's update s->scr */
	if (s->sel-1<s->scr) s->scr=s->sel-1;
	if (s->sel+1>=s->scr+s->h) s->scr=s->sel+1-s->h+1;
	while (s->scr+s->h>=n) s->scr--;
	if (s->scr<0) s->scr=0;				// this is what happens if s->h==s->n
}

void ScrollChgWrapOnlyAtEnd(SCROLL *s,int d) {
	/* likes ScrollChg, but prevents selection from wrapping if cursor was not at end */
	int n=s->n,sel2=s->sel+d;
	if (sel2<0) { if (s->sel>0) sel2=0; else sel2=-1; }
	else if (sel2>=n) { if (s->sel<n-1) sel2=n-1; else sel2=n; }
	s->sel=sel2;
}

void strtolower(char *s) {
	while (*s) *s=tolower(*s),s++;
}

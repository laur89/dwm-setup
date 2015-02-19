static void
bstack(Monitor *m) {
	int w, h, mh, mx, tx, ty, tw, r;
	unsigned int i, n;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if(n == 0)
		return;
	if(n > m->nmaster) {
		mh = m->nmaster ? m->mfact * m->wh : 0;
		tw = m->ww / (n - m->nmaster);
		ty = m->wy + mh;
	}
	else {
		mh = m->wh;
		tw = m->ww;
		ty = m->wy;
	}
	for(i = mx = 0, tx = m->wx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++, r=0) {
        if(n == 1) {
            if (c->bw) {
                /*[> remove border when only one window is on the current tag <]*/
                c->oldbw = c->bw;
                c->bw = 0;
                r = 1;
            }
        }
        else if(!c->bw && c->oldbw) {
            /*[> restore border when more than one window is displayed <]*/
            c->bw = c->oldbw;
            c->oldbw = 0;
            r = 1;
        }

		if(i < m->nmaster) {
			w = (m->ww - mx) / (MIN(n, m->nmaster) - i);
			resize(c, m->wx + mx, m->wy, w - (2 * c->bw), mh - (2 * c->bw), False);
			mx += WIDTH(c);
		}
		else {
			h = m->wh - mh;
			resize(c, tx, ty, tw - (2 * c->bw), h - (2 * c->bw), False);
			if(tw != m->ww)
				tx += WIDTH(c);
		}
	}
}

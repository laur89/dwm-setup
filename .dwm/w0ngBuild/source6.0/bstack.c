static void
bstack(Monitor *m) {
	unsigned int w, h, mh, mx, tx, ty, tw, r;
    float mfacts = 0, sfacts = 0;
	unsigned int i, n;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) {
		if(n < m->nmasters[m->curtag])
			mfacts += c->cfact;
		else
			sfacts += c->cfact;
	}

	if(n == 0)
		return;
	if(n > m->nmasters[m->curtag]) { // if more clients than master slots
		mh = m->nmasters[m->curtag] ? m->mfacts[m->curtag] * m->wh : 0; // master height?
		tw = m->ww / (n - m->nmasters[m->curtag]); // stack width? // !!! TODO: stacki width leidmiseks jagatakse julmalt slave'ide arvuga!!
		ty = m->wy + mh; // stack y?
	} else {
		mh = m->wh; // master height?
		tw = m->ww;
		ty = m->wy;
	}

	for(i = mx = tx = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++, r=0) {
        if (n == 1) {
            if (c->bw) {
                /*[> remove border when only one window is on the current tag <]*/
                c->oldbw = c->bw;
                c->bw = 0;
                r = 1;
            }
        } else if(!c->bw && c->oldbw) {
            /*[> restore border when more than one window is displayed <]*/
            c->bw = c->oldbw;
            c->oldbw = 0;
            r = 1;
        }

		if (i < m->nmasters[m->curtag]) {
			/*w = (m->ww - mx) / (MIN(n, m->nmasters[m->curtag]) - i);*/
			w = (m->ww - mx) * (c->cfact / mfacts);
			resize(c,
                   m->wx + mx,
                   m->wy,
                   w - (2 * c->bw),
                   mh - (2 * c->bw),    False);
			mx += WIDTH(c);
            mfacts -= c->cfact;
		} else {
            h = m->wh - mh;
			w = (m->ww - tx) * (c->cfact / sfacts);
			resize(c,
                   m->wx + tx,
                   ty,
                   w - (2 * c->bw),
                   h - (2 * c->bw),    False);
			if(tw != m->ww) // TODO: what's this for? doesn't seem to be necessary; it simply checks if found stack width != window width, meaning there are more than 1 slaves;
				tx += WIDTH(c);
            sfacts -= c->cfact;
		}
	}
}

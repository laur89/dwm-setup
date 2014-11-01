static void
bstack(Monitor *m) {
	unsigned int i, n, w, mh, mx, tx;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if(n == 0)
		return;

	if(n > m->nmasters[m->curtag])
		mh = m->nmasters[m->curtag] ? m->wh * m->mfacts[m->curtag] : 0;
	else
		mh = m->wh;
	for(i = mx = tx = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if(i < m->nmasters[m->curtag]) {
			w = (m->ww - mx) / (MIN(n, m->nmasters[m->curtag]) - i);
			resize(c, m->wx + mx, m->wy, w - (2*c->bw), mh - (2*c->bw), False);
			mx += WIDTH(c) + 2 * globalborder;	//uselessgaps modified
		}
		else {
			w = (m->ww - tx) / (n - i);
			resize(c, m->wx + tx, m->wy + mh, w - (2*c->bw), m->wh - mh - (2*c->bw), False);
				tx += WIDTH(c) + 2 * globalborder;	//uselessgaps modified
		}
}

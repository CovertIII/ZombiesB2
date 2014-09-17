struct Chain {
    int num;
    int ppl[100];
    b2Body * links[100];
    b2Joint * jt[200];

    int make_q[100];
    int qnum;

    b2Body * delete_q[100];
    int dnum;
};


void chain_remove(game gm, int index);

void chain_ready_zero(game gm);

void chain_cut(game gm, int index);

void chain_update(game gm);

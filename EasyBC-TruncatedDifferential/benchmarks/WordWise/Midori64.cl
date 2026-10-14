@cipher Midori64

sbox uint4[16] s = {12, 10, 13, 3, 14, 11, 15, 7, 8, 9, 1, 5, 0, 2, 4, 6};

pbox uint[16] p = {0,7,14,9,5,2,11,12,15,8,1,6,10,13,4,3};

pboxm uint4[4][4] M = {{0,1,1,1},{1,0,1,1},{1,1,0,1},{1,1,1,0}};

ffm uint4[16][16] M = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                      {0, 2, 4, 6, 8, 10, 12, 14, 3, 1, 7, 5, 11, 9, 15, 13},
                      {0, 3, 6, 5, 12, 15, 10, 9, 11, 8, 13, 14, 7, 4, 1, 2},
                      {0, 4, 8, 12, 3, 7, 11, 15, 6, 2, 14, 10, 5, 1, 13, 9},
                      {0, 5, 10, 15, 7, 2, 13, 8, 14, 11, 4, 1, 9, 12, 3, 6},
                      {0, 6, 12, 10, 11, 13, 7, 1, 5, 3, 9, 15, 14, 8, 2, 4},
                      {0, 7, 14, 9, 15, 8, 1, 6, 13, 10, 3, 4, 2, 5, 12, 11},
                      #{0, 6, 12, 10, 11, 13, 7, 1, 5, 3, 9, 15, 14, 8, 2, 4},
                      {0, 8, 3, 11, 6, 14, 5, 13, 12, 4, 15, 7, 10, 2, 9, 1},
                      {0, 9, 1, 8, 2, 11, 3, 10, 4, 13, 5, 12, 6, 15, 7, 14},
                      {0, 10, 7, 13, 14, 4, 9, 3, 15, 5, 8, 2, 1, 11, 6, 12},
                      {0, 11, 5, 14, 10, 1, 15, 4, 7, 12, 2, 9, 13, 6, 8, 3},
                      {0, 12, 11, 7, 5, 9, 14, 2, 10, 6, 1, 13, 15, 3, 4, 8},
                      {0, 13, 9, 4, 1, 12, 8, 5, 2, 15, 11, 6, 3, 14, 10, 7},
                      {0, 14, 15, 1, 13, 3, 2, 12, 9, 7, 6, 8, 4, 10, 11, 5},
                      {0, 15, 13, 2, 9, 6, 4, 11, 1, 14, 12, 3, 8, 7, 5, 10}};

r_fn uint4[16] round_function1(uint8 r, uint4[16] key, uint4[16] input) {
    uint4[16] s_out;

    for (i from 0 to 15) {
        s_out[i] = s<input[i]>;
   }

    uint4[16] p_out = p<s_out>;

    uint4[16] m_out;
    for (i from 0 to 3) {
        uint4[4] mtemp = View(p_out, i*4, i*4+3);
        uint4[4] sm_out = M * mtemp;

        m_out[i*4] = sm_out[0];
        m_out[i*4 + 1] = sm_out[1];
        m_out[i*4 + 2] = sm_out[2];
        m_out[i*4 + 3] = sm_out[3];
    }

    for (i from 0 to 15) {
        m_out[i] = m_out[i] ^ key[i];
    }

    return m_out;
}

r_fn uint4[16] round_function2(uint8 r, uint4[16] key, uint4[16] input) {
    uint4[16] s_out;

    for (i from 0 to 15) {
        s_out[i] = s<input[i]>;
    }

    s_out = s_out ^ key;
    return s_out;
}

fn uint4[16] enc(uint4[256] key, uint4[16] r_plaintext){
    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }
    for (i from 1 to 15) {
        r_plaintext = round_function1(i, View(key, i * 16, ( i + 1 ) * 16 - 1), r_plaintext);
    }
    r_plaintext =round_function2(16, View(key, 0, 15), r_plaintext);
    return r_plaintext;
}
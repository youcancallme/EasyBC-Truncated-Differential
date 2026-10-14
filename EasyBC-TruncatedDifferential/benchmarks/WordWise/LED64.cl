@cipher LED64

sbox uint4[16] s = {
    12, 5, 6, 11,
    9, 0, 10, 13,
    3, 14, 15, 8,
    4, 7, 1, 2
};


pbox uint[16] p = {
    0, 1, 2, 3,
    5, 6, 7, 4,
    10, 11, 8, 9,
    15, 12, 13, 14
};


pboxm uint4[4][4] M = {
    {4,  1,  2,  2},
    {8,  6,  5,  6},
    {11, 14, 10, 9},
    {2,  2, 15, 11}
};


ffm uint4[16][16] M = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},

    {0, 2, 4, 6, 8, 10, 12, 14, 3, 1, 7, 5, 11, 9, 15, 13},

    {0, 3, 6, 5, 12, 15, 10, 9, 11, 8, 13, 14, 7, 4, 1, 2},

    {0, 4, 8, 12, 3, 7, 11, 15, 6, 2, 14, 10, 5, 1, 13, 9},

    {0, 5, 10, 15, 7, 2, 13, 8, 14, 11, 4, 1, 9, 12, 3, 6},

    {0, 6, 12, 10, 11, 13, 7, 1, 5, 3, 9, 15, 14, 8, 2, 4},

    {0, 7, 14, 9, 15, 8, 1, 6, 13, 10, 3, 4, 2, 5, 12, 11},

    {0, 8, 3, 11, 6, 14, 5, 13, 12, 4, 15, 7, 10, 2, 9, 1},

    {0, 9, 1, 8, 2, 11, 3, 10, 4, 13, 5, 12, 6, 15, 7, 14},

    {0, 10, 7, 13, 14, 4, 9, 3, 15, 5, 8, 2, 1, 11, 6, 12},

    {0, 11, 5, 14, 10, 1, 15, 4, 7, 12, 2, 9, 13, 6, 8, 3},

    {0, 12, 11, 7, 5, 9, 14, 2, 10, 6, 1, 13, 15, 3, 4, 8},

    {0, 13, 9, 4, 1, 12, 8, 5, 2, 15, 11, 6, 3, 14, 10, 7},

    {0, 14, 15, 1, 13, 3, 2, 12, 9, 7, 6, 8, 4, 10, 11, 5},

    {0, 15, 13, 2, 9, 6, 4, 11, 1, 14, 12, 3, 8, 7, 5, 10}
};



r_fn uint4[16] round_function(
    uint8 r,
    uint4[4] key,
    uint4[16] input
) {


    input[0] = input[0] ^ 4;
    input[1] = input[1] ^ key[0];

    input[4] = input[4] ^ 5;
    input[5] = input[5] ^ key[1];

    input[8] = input[8] ^ 2;
    input[9] = input[9] ^ key[0];

    input[12] = input[12] ^ 3;
    input[13] = input[13] ^ key[1];

    uint4[16] s_out;

    for (i from 0 to 15) {
        s_out[i] = s<input[i]>;
    }
    uint4[16] p_out = p<s_out>;

    uint4[16] m_out;

    for (i from 0 to 3) {

        uint4[4] mtemp = {
            p_out[i],
            p_out[i + 4],
            p_out[i + 8],
            p_out[i + 12]
        };

        uint4[4] sm_out = M * mtemp;

        m_out[i] = sm_out[0];
        m_out[i + 4] = sm_out[1];
        m_out[i + 8] = sm_out[2];
        m_out[i + 12] = sm_out[3];
    }


    return m_out;
}


fn uint4[16] enc(
    uint4[16] key,
    uint4[16] r_plaintext
) {


    uint4[4] rc1  = {0, 1, 0, 0};
    uint4[4] rc2  = {0, 3, 0, 0};
    uint4[4] rc3  = {0, 7, 0, 0};
    uint4[4] rc4  = {1, 7, 0, 0};

    uint4[4] rc5  = {3, 7, 0, 0};
    uint4[4] rc6  = {7, 6, 0, 0};
    uint4[4] rc7  = {7, 5, 0, 0};
    uint4[4] rc8  = {7, 3, 0, 0};

    uint4[4] rc9  = {6, 7, 0, 0};
    uint4[4] rc10 = {5, 7, 0, 0};
    uint4[4] rc11 = {3, 6, 0, 0};
    uint4[4] rc12 = {7, 4, 0, 0};

    uint4[4] rc13 = {7, 1, 0, 0};
    uint4[4] rc14 = {6, 3, 0, 0};
    uint4[4] rc15 = {4, 7, 0, 0};
    uint4[4] rc16 = {1, 6, 0, 0};

    uint4[4] rc17 = {3, 5, 0, 0};
    uint4[4] rc18 = {7, 2, 0, 0};
    uint4[4] rc19 = {6, 5, 0, 0};
    uint4[4] rc20 = {5, 3, 0, 0};

    uint4[4] rc21 = {2, 6, 0, 0};
    uint4[4] rc22 = {5, 4, 0, 0};
    uint4[4] rc23 = {3, 0, 0, 0};
    uint4[4] rc24 = {6, 0, 0, 0};

    uint4[4] rc25 = {4, 1, 0, 0};
    uint4[4] rc26 = {0, 2, 0, 0};
    uint4[4] rc27 = {0, 5, 0, 0};
    uint4[4] rc28 = {1, 3, 0, 0};

    uint4[4] rc29 = {2, 7, 0, 0};
    uint4[4] rc30 = {5, 6, 0, 0};
    uint4[4] rc31 = {3, 4, 0, 0};
    uint4[4] rc32 = {7, 0, 0, 0};


    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }

    r_plaintext = round_function(1, rc1, r_plaintext);
    r_plaintext = round_function(2, rc2, r_plaintext);
    r_plaintext = round_function(3, rc3, r_plaintext);
    r_plaintext = round_function(4, rc4, r_plaintext);


    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }

    r_plaintext = round_function(5, rc5, r_plaintext);
    r_plaintext = round_function(6, rc6, r_plaintext);
    r_plaintext = round_function(7, rc7, r_plaintext);
    r_plaintext = round_function(8, rc8, r_plaintext);


    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }

    r_plaintext = round_function(9, rc9, r_plaintext);
    r_plaintext = round_function(10, rc10, r_plaintext);
    r_plaintext = round_function(11, rc11, r_plaintext);
    r_plaintext = round_function(12, rc12, r_plaintext);


    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }

    r_plaintext = round_function(13, rc13, r_plaintext);
    r_plaintext = round_function(14, rc14, r_plaintext);
    r_plaintext = round_function(15, rc15, r_plaintext);
    r_plaintext = round_function(16, rc16, r_plaintext);


    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }

    r_plaintext = round_function(17, rc17, r_plaintext);
    r_plaintext = round_function(18, rc18, r_plaintext);
    r_plaintext = round_function(19, rc19, r_plaintext);
    r_plaintext = round_function(20, rc20, r_plaintext);


    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }

    r_plaintext = round_function(21, rc21, r_plaintext);
    r_plaintext = round_function(22, rc22, r_plaintext);
    r_plaintext = round_function(23, rc23, r_plaintext);
    r_plaintext = round_function(24, rc24, r_plaintext);


    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }

    r_plaintext = round_function(25, rc25, r_plaintext);
    r_plaintext = round_function(26, rc26, r_plaintext);
    r_plaintext = round_function(27, rc27, r_plaintext);
    r_plaintext = round_function(28, rc28, r_plaintext);


    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }

    r_plaintext = round_function(29, rc29, r_plaintext);
    r_plaintext = round_function(30, rc30, r_plaintext);
    r_plaintext = round_function(31, rc31, r_plaintext);
    r_plaintext = round_function(32, rc32, r_plaintext);



    for (i from 0 to 15) {
        r_plaintext[i] = r_plaintext[i] ^ key[i];
    }


    return r_plaintext;
}
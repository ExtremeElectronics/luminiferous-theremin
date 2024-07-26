//mapping distance to frequency
//from distarray.py
// with MINDISTANCE 20 and MAXDISTANCE 350

uint16_t farray[320]={
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,78,78,79,80,80,82,82,83,84,85,87,87,
    88,89,90,92,93,94,95,96,98,99,100,101,102,104,105,106,
    107,108,110,111,112,113,115,117,118,119,120,121,123,124,125,127,
    128,131,132,133,135,136,139,140,141,143,144,147,148,150,151,153,
    156,157,159,160,162,165,166,168,170,172,175,176,178,180,182,185,
    187,189,191,193,196,198,200,202,204,208,210,212,214,216,220,222,
    224,227,229,233,235,238,240,243,247,249,252,255,257,262,264,267,
    270,272,277,280,283,286,289,294,297,300,303,306,311,314,317,321,
    324,330,333,336,340,343,349,352,356,360,364,370,374,378,382,386,
    392,396,400,404,408,415,419,424,428,433,440,444,449,454,458,466,
    471,476,481,486,494,499,504,509,515,523,528,534,539,545,554,560,
    566,572,578,587,593,599,606,612,622,628,635,642,648,659,666,673,
    680,687,698,705,713,720,728,740,748,756,764,772,784,792,801,809,
    818,831,839,848,857,866,880,889,898,908,917,932,942,952,962,972,
    988,998,1009,1020,1030,1047,1058,1069,1080,1092,1109,1121,1133,1145,1157,1175,
    1187,1200,1213,1225,1245,1258,1271,1285,1298,1319,1333,1347,1361,1375,1397,1412,
    1427,1442,1457,1480,1496,1512,1528,1544,1568,1584,1601,1618,1635,1661,1679,1697,
    1715,1733,1760,1779,1798,1817,1836,1865,1885,1905,1925,1945,1976,1997,2018,2039,
    2061,2093,2115,2138,2160,2183,2217,2241,2265,2289,2313,2349,2373,2397,2421,0,
    
};

//Wobulator Sine
const uint8_t msign[64] = {
    0x7f,  0x8b,  0x98,  0xa4,
    0xb0,  0xbb,  0xc6,  0xd0,
    0xd9,  0xe2,  0xe9,  0xef,
    0xf5,  0xf9,  0xfc,  0xfe,
    0xff,  0xfe,  0xfc,  0xf9,
    0xf5,  0xef,  0xe9,  0xe2,
    0xd9,  0xd0,  0xc6,  0xbb,
    0xb0,  0xa4,  0x98,  0x8b,
    0x7f,  0x73,  0x66,  0x5a,
    0x4e,  0x43,  0x38,  0x2e,
    0x25,  0x1c,  0x15,  0x0f,
    0x09,  0x05,  0x02,  0x00,
    0x00,  0x00,  0x02,  0x05,
    0x09,  0x0f,  0x15,  0x1c,
    0x25,  0x2e,  0x38,  0x43,
    0x4e,  0x5a,  0x66,  0x73
};

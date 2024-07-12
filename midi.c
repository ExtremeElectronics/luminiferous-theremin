
#define MNOTES 60
#define MIDIACC 2
#define MIDINEAR 8
#define NONOTEFOUND 0x9999

uint16_t mfrequency[MNOTES]={
    78,82,87,92,98,104,110,117,123,131,139,147,
    156,165,175,185,196,208,220,233,247,262,277,294,
    311,330,349,370,392,415,440,466,494,523,554,587,
    622,659,698,740,784,831,880,932,988,1047,1109,1175,
    1245,1319,1397,1480,1568,1661,1760,1865,1976,2093,2217,
    2349
};

char mnotes[MNOTES][6]={
    "D2#\0","E2\0" ,"F2\0", "F2#\0", "G2\0", "G2#\0", "A2\0",  "A2#\0", "B2\0",  "C3\0",  "C3#\0", "D3\0",  "D3#\0", "E3\0",
    "F3\0" ,"F3#\0","G3\0", "G3#\0", "A3\0", "A3#\0", "B3\0",  "C4\0",  "C4#\0", "D4\0",  "D4#\0", "E4\0",  "F4\0", "F4#\0",
    "G4\0" ,"G4#\0","A4\0", "A4#\0", "B4\0", "C5\0",  "C5#\0", "D5\0",  "D5#\0", "E5\0",  "F5\0",  "F5#\0", "G5\0", "G5#\0",
    "A5\0" ,"A5#\0","B5\0", "C6\0",  "C6#\0","D6\0",  "D6#\0", "E6\0",  "F6\0",  "F6#\0", "G6\0",  "G6#\0", "A6\0", "A6#\0",
    "B7\0" ,"C7\0" ,"C7#\0","D7\0"
};

//retunrs midi note if within MIDIACC or -1
int getMidiNote(uint16_t f){
   int x;
   for (x=0;x<MNOTES;x++){
      if(f<=mfrequency[x]+MIDIACC) break;
   }
   if((f>=mfrequency[x]-MIDIACC) && (f<=mfrequency[x]+MIDIACC) ){
     return x;
   }else{
     return -1;
   }      
}

//returns frequency of nearest note if within MIDINEAR
int nearestNote(uint16_t f){
    int x;
    int r=NONOTEFOUND;
    for (x=0;x<MNOTES;x++){
//       if(f<=mfrequency[x]+MIDINEAR){
       if((f>=mfrequency[x]-MIDINEAR) && (f<=mfrequency[x]+MIDINEAR) ){
           r=mfrequency[x];
           break;
       }   
    }
    return r;

}
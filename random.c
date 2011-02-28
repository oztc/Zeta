// The random-module is based on the "Fast Uniform Random Number Generator"
// introduced by G. Marsaglia. The hopefully easy to use "shell"
// for Marsaglias algorithm was programmed by Heinz van Saanen. The
// random-module comes with ABSOLUTELY NO WARRANTY.

#include <math.h>

#define TRUE    1
#define FALSE   0

static float u[97], c, cd, cm;
static int i97, j97;
static char test=FALSE;


// ============================================================
// ==================== INTITIALIZATIONS ======================
// ============================================================

void sran(int ij, int kl) {
    float s, t;
    int i, j, k, l, m;
    int ii, jj;
    if(ij<0 || ij>31328) {
        ij=0;
        kl=0;
    }
    if(kl<0 || kl>30081) {
        ij=0;
        kl=0;
    }
    i=(int)fmodf(ij/177.0f , 177.0f)+2;
    j=(int)fmodf((float)ij , 177.0f)+2;
    k=(int)fmodf(kl/169.0f , 178.0f)+1;
    l=(int)fmodf((float)kl , 169.0f);
    for(ii=0; ii<=96; ii++) {
        s=0.0f;
        t=0.5f;
        for(jj=0; jj<=23; jj++) {
            m= (int)fmodf(fmodf((float)(i*j), 179.0f)*k, 179.0f);
            i=j;
            j=k;
            k=m;
            l=(int)fmodf(53.0f*l+1.0f , 169.0f);
            if(fmodf((float)(l*m), 64.0f) >= 32.0f) s+=t;
            t*=0.5f;
        }
        *(u+ii)=s;
    }
    c  =   362436.0f / 16777216.0f;
    cd =  7654321.0f / 16777216.0f;
    cm = 16777213.0f / 16777216.0f;
    i97=96;
    j97=32;
    test=TRUE;
}


// ============================================================
// ====================== CORE FUNCTION =======================
// ============================================================

float ranf(void) {
    float uni;
    if(!test) {
        sran(0,0);
        test=TRUE;
    }
    uni=*(u+i97)-*(u+j97);
    if(uni<0.0f)
        uni+=1.0f;
    *(u+i97--)=uni;
    if(i97<0)
        i97=96;
    j97--;
    if(j97<0)
        j97=96;
    c-=cd;
    if(c<0.0f)
        c+=cm;
    uni-=c;
    if(uni<0.0f)
        uni+=1.0f;
    return uni;
}


// ============================================================
// ==================== "SHELL"-FUNCTIONS  ====================
// ============================================================

unsigned char ran1(void) {
    static int r, pass=23;
    if(pass<0) pass=23;
    if(pass==23) r=(16777216.0f*ranf()); //ran24();
    if(r&(1<<(pass--))) return (1); else return (0);
}


unsigned char ran8(void) {
    static int r, pass=2;
    if(pass<0) pass=2;
    if(pass==2) r=(16777216.0f*ranf()); //ran24();
    switch(pass) {
        case 2: pass--; return (r>>16); break;
        case 1: pass--; return ((r&0x0000FF00)>>8); break;
        case 0: pass--; return (r&0x000000FF); break;
    }
    return 0;
}


unsigned short int ran16(void) {
    static int r, h, pass=2;
    if(pass<0) pass=2;
    switch(pass) {
        case 2:	pass--;
                r=(16777216.0f*ranf()); //ran24();
                return (r>>8);
                break;
        case 1:	pass--;
                h=((r<<8)&0x0000FF00);
                r=(16777216.0f*ranf()); //ran24();
                return (h|(r>>16));
                break;
        case 0:	pass--;
                return (r&0x0000FFFF);
                break;
    }
    return 0;
}


unsigned int ran24(void) {
    return ((unsigned int)(16777216.0f*ranf()));
}


unsigned int ran32(void) {
    unsigned int i;
    i=((unsigned int)(16777216.0f*ranf()))<<8;
    i|=(unsigned int)ran8();
    return (i);
}


unsigned long long ran64(void) {
    unsigned long long i;
    i=((unsigned long long)(16777216.0f*ranf()))<<24;
    i|=(unsigned long long)(16777216.0f*ranf());
    i=(i<<16)|((unsigned long long)ran16());
    return (i);
}


// ============================================================
// ====================== TESTROUTINE =========================
// ============================================================

unsigned char rantest(void) {
    int i;
    unsigned char result=TRUE;
    sran(1802,9373);                       // Initializing
    for(i=0; i<20000; i++) ran24();        // 20.000 Calls
    if(ran24() !=  6533892) return FALSE;  // 6 Tests
    if(ran24() != 14220222) return FALSE;
    if(ran24() !=  7275067) return FALSE;
    if(ran24() !=  6172232) return FALSE;
    if(ran24() !=  8354498) return FALSE;
    if(ran24() != 10633180) return FALSE;
    return result;
}

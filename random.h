// GENERAL NOTES ON THE MODULE RANDOM:
//
// The random-module is based on the "Fast Uniform Random Number Generator"
// introduced by G. Marsaglia. The algorithm basically generates a sequence
// of 32-bit-floating-point random numbers uniformly distributed in the
// interval (0,1), the end points excluded. It is a very! good compromise
// beetween speed and "random-capabilities". It passes most of the common
// tests for "randomness" and has a more than sufficient period-lenght.
// Nevertheless it should not be used for really super "strong strong"
// cryptographical purposes. In this field there are even more clever -
// and much slower - algorithms available. The hopefully easy to use "shell"
// for Marsaglias algorithm was programmed by Heinz van Saanen. The
// random-module comes with ABSOLUTELY NO WARRANTY.
//
// WHAT FOR?
//
// The system-based random-generators in different operation systems and
// compilers are relatively "weak". Probably you will miss a "normal"
// distribution or a long period length, mostly both of it. Unless you
// are not dealing with extreme belongings, the Marsaglia-algorithm should
// fullfill your needs.
//
// DETAILS:
//
// The random number sequences created by two seeds ij and kl are of sufficient
// length to complete an entire calculation with. For example, if several
// different groups are working on different parts of the same calculation,
// each group could be assigned its own ij seed. This would leave each group
// with over 30000 choices for the second seed. That is to say, this random
// number generator can create 900 million different subsequences -- with
// each subsequence having a length of at least approximately 10^30.
//
// TESTING:
//
// Set ij = 1802 & kl = 9373 for testing the random number generator, as it
// is automatically done by rantest(). The subroutine rantest() will generate
// 20.000 consecutive random numbers. Then it checks the next six random
// numbers generated multiplied by 4096*4096. If the random number generator
// works, what rantest() proves automatically, the following random numbers
// should be: 6533892,14220222,7275067,6172232,8354498 and 10633180
//
// INITIALIZING:
//
// Set the seeds with sran(int ij, int kl). The conditions for ij and kl are:
// 0<=ij<=31328 und 0<=kl<=30081
//
// RETURN-TYPES:
//
// ranf()    : float              0.0f .. 1.0f
// ran8()    : unsigned char      0 ..                        255 = 2^ 8-1
// ran16()   : unsigned short int 0 ..                     65.535 = 2^16-1
// ran24()   : unsigned int       0 ..                 16.777.215 = 2^24-1
// ran32()   : unsigned int       0 ..              4.294.967.295 = 2^32-1
// ran64()   : unsigned long long 0 .. 18.446.744.073.709.551.615 = 2^64-1
// rantest() : unsigned char	  0 || 1
//
// ERROR-HANDLING:
//
// Any ranx()-call without prior initialization : automatic sran(0,0)
// Seeds out of limits                          : automatic ij=kl=0
//
// *************************************************************************

void                sran(int ij, int kl);
float               ranf(void);
unsigned char       ran1(void);
unsigned char       ran8(void);
unsigned short int  ran16(void);
unsigned int        ran24(void);
unsigned int        ran32(void);
unsigned long long  ran64(void);
unsigned char       rantest(void);


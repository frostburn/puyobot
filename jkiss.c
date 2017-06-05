/* Public domain code for JKISS RNG */
// http://www0.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct jkiss32
{
    unsigned int x, y, z, c;
} jkiss32;

typedef struct jkiss64
{
    unsigned long long x, y;
    unsigned int z, c;
    unsigned int zl, cl;  // Only used when generating 64 bit results directly.
} jkiss64;

unsigned int devrand(void)
{
    int fn;
    unsigned int r;
    fn = open("/dev/urandom", O_RDONLY);
    if (fn == -1)
        exit(-1); /* Failed! */
    if (read(fn, &r, 4) != 4)
        exit(-1); /* Failed! */
    close(fn);
    return r;
}

/* Initialise KISS generator using /dev/urandom */
void jkiss32_init(jkiss32 *j)
{
    j->x = devrand();
    while (!(j->y = devrand())); /* y must not be zero! */
    j->z = devrand();
    /* We don’t really need to set c as well but let's anyway… */
    /* NOTE: offset c by 1 to avoid z=c=0 */
    j->c = devrand() % 698769068 + 1; /* Should be less than 698769069 */
}

void jkiss64_init(jkiss64 *j)
{
    j->x = devrand();
    j->x = (j->x << 32) | devrand();
    while (!(j->y = devrand())); /* y must not be zero! */
    j->y = (j->y << 32) | devrand();
    j->z = devrand();
    j->zl = devrand();
    /* We don’t really need to set c as well but let's anyway… */
    /* NOTE: offset c by 1 to avoid z=c=0 */
    j->c = devrand() % 698769068 + 1; /* Should be less than 698769069 */
    j->cl = devrand() % 698769068 + 1;
}

// Fastest
unsigned int jkiss32_step(jkiss32 *j)
{
    unsigned long long t;
    j->x = 314527869 * j->x + 1234567;
    j->y ^= j->y << 5; j->y ^= j->y >> 7; j->y ^= j->y << 22;
    t = 4294584393ULL * j->z + j->c;
    j->c = t >> 32;
    j->z = t;
    return j->x + j->y + j->z;
}

// Almost as fast, but with a longer period
unsigned int jkiss64_step(jkiss64 *j)
{
    unsigned long long t;
    j->x = 1490024343005336237ULL * j->x + 123456789;
    j->y ^= j->y << 21; j->y ^= j->y >> 17; j->y ^= j->y << 30;
    t = 4294584393ULL * j->z + j->c; j->c = t >> 32; j->z = t;
    return (unsigned int)(j->x >> 32) + (unsigned int)j->y + j->z; /* Return 32-bit result */
}

// Generates 64-bit results
unsigned long long jkiss64_step_long(jkiss64 *j)
{
    unsigned long long t;
    j->x = 1490024343005336237ULL * j->x + 123456789;
    j->y ^= j->y << 21; j->y ^= j->y >> 17; j->y ^= j->y << 30;
    t = 4294584393ULL * j->z + j->c; j->c = t >> 32; j->z = t;
    t = 4246477509ULL * j->zl + j->cl; j->cl = t >> 32; j->zl = t;
    return j->x + j->y + j->z + ((unsigned long long)j->zl << 32); /* Return 64-bit result */
}

static jkiss32 global_gen32 = {123456789, 987654321, 43219876, 6543217};
static jkiss64 global_gen64 = {123456789123ULL, 987654321987ULL, 43219876, 6543217, 21987643, 1732654};


#ifdef _OPENMP
    #include <omp.h>
    #define OMP_MAX_THREADS (32)
    static jkiss64 omp_gen64[OMP_MAX_THREADS];
    unsigned int jrand_omp() {
        return jkiss64_step(omp_gen64 + omp_get_thread_num());
    }
    void _jkiss_init_omp() {
        for (int i = 0; i < OMP_MAX_THREADS; ++i) {
            jkiss64_init(omp_gen64 + i);
        }
    }
#else
    void _jkiss_init_omp() {}
#endif

void jkiss_init() {
    jkiss32_init(&global_gen32);
    jkiss64_init(&global_gen64);
    _jkiss_init_omp();
}

void jkiss_seed(unsigned int seed) {
    global_gen32 = (jkiss32){seed, seed*seed, seed*seed*seed, seed ^ (seed >> 11)};
    for (int i = 0; i < 100; ++i) {
        jkiss32_step(&global_gen32);
    }
    global_gen64 = (jkiss64){
        jkiss32_step(&global_gen32),
        jkiss32_step(&global_gen32),
        jkiss32_step(&global_gen32),
        jkiss32_step(&global_gen32),
        jkiss32_step(&global_gen32),
        jkiss32_step(&global_gen32)
    };
    global_gen64.x = global_gen64.x << 32 | jkiss32_step(&global_gen32);
    global_gen64.y = global_gen64.y << 32 | jkiss32_step(&global_gen32);
}

unsigned int jrand() {
    return jkiss64_step(&global_gen64);
}

unsigned long long jlrand() {
    return jkiss64_step_long(&global_gen64);
}

double jdrand() {
    return jkiss64_step_long(&global_gen64) / 18446744073709551615.0;
}

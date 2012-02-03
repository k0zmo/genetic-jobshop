#include "Random.h"
#include <climits>

namespace core
{
	// -------------------------------------------------------------------------
	class Random_pimpl
	{
	public:
		virtual ~Random_pimpl() {}
		virtual uint32 rand_impl() = 0;
		virtual void srand_impl(uint32 seed) = 0;
	};

	// Generator liniowy, implementacja wbudowanego rand()
	class LCGenerator : public Random_pimpl
	{
	public:
		LCGenerator() : _a(214013L), _c(2531011L), seed(0) {}
		LCGenerator(int32 a, int32 c) : _a(a), _c(c) {}
		virtual ~LCGenerator() {}

		virtual uint32 rand_impl();
		virtual void srand_impl(uint32 seed);

	private:
		uint32 _a, _c;
		uint32 seed;
	};

	// implementacja: Mersenne Twister Generator : http://en.wikipedia.org/wiki/Mersenne_twister
	class MTGenerator : public Random_pimpl
	{
	public:
		MTGenerator() : idx(0) {}
		virtual ~MTGenerator() {}

		virtual uint32 rand_impl();
		virtual void srand_impl(uint32 seed);

	private:
		enum { N = 624 };
		enum { M = 397 };

		uint32 state[N];
		int idx;

		void reload(void);
	};

	// implementacja: http://burtleburtle.net/bob/rand/isaac.html
	class ISAACGenerator : public Random_pimpl
	{
	public:
		ISAACGenerator() {}
		virtual ~ISAACGenerator() {}

		virtual uint32 rand_impl();
		virtual void srand_impl(uint32 seed);
		virtual void srand_impl(uint32 a = 0, uint32 b = 0, uint32 c = 0, uint32* s = 0);

	private:
		enum { RANDSIZL = 8 };
		enum { N = 1 << RANDSIZL };

		// kontekst generatora
		struct randctx
		{
			uint32 randcnt;
			uint32 randrsl[N];
			uint32 randmem[N];
			uint32 randa;
			uint32 randb;
			uint32 randc;
		};
		randctx ctx;

		void randinit(void);
		void isaac(void);

		inline void shuffle(uint32& a, uint32& b, uint32& c,
			uint32& d, uint32& e, uint32& f, uint32& g, uint32& h);
		inline uint32 ind(uint32* mm, uint32 x);
		inline void rngstep(uint32 mix, uint32& a, uint32& b, uint32*& mm,
			uint32*& m, uint32*& m2, uint32*& r, uint32& x, uint32& y);
	};

	// -------------------------------------------------------------------------
	Random::Random()
	: mpGenerator(0), a(0), b((uint32)0xFFFFFFFF)
	{
	}
	// -------------------------------------------------------------------------
	Random::~Random()
	{
		if(mpGenerator)
		{
			delete mpGenerator;
			mpGenerator = 0;
			a = b = 0;
		}
	}
	// -------------------------------------------------------------------------
	void Random::setGenerator(EGeneratorType type)
	{
		if(mpGenerator)
			delete mpGenerator;

		switch(type)
		{
		case GT_LINEAR:
			mpGenerator = new LCGenerator();
			break;
		case GT_MERSENNE_TWISTER:
			mpGenerator = new MTGenerator();
			break;
		case GT_ISAAC:
			mpGenerator = new ISAACGenerator();
			break;
		}

	}
	// -------------------------------------------------------------------------
	void Random::srand(uint32 seed)
	{
		assert(mpGenerator);

		mpGenerator->srand_impl(seed);
	}
	// -------------------------------------------------------------------------
	void Random::setBound(uint32 a, uint32 b)
	{
		this->a = a;
		this->b = b;
	}
	// -------------------------------------------------------------------------
	uint32 Random::random()
	{
		assert(a < b);
		assert(mpGenerator);

		register uint32 used = b - a;
		used |= used >> 1;
		used |= used >> 2;
		used |= used >> 4;
		used |= used >> 8;
		used |= used >> 16;

		uint32 i;
		do
		{
			i = mpGenerator->rand_impl() & used;
		} while(i > b - a);
		return i + a;
	}
	// -------------------------------------------------------------------------
	uint32 Random::random(uint32 a, uint32 b)
	{
		assert(a < b);
		assert(mpGenerator);

		register uint32 used = b - a;
		used |= used >> 1;
		used |= used >> 2;
		used |= used >> 4;
		used |= used >> 8;
		used |= used >> 16;

		uint32 i;
		do
		{
			i = mpGenerator->rand_impl() & used;
		} while(i > b - a);
		return i + a;
	}	
	// -------------------------------------------------------------------------
	float Random::randomUnorm()
	{
		assert(mpGenerator);
		uint32 i = mpGenerator->rand_impl();

		return static_cast<float>(i) / static_cast<float>(static_cast<uint32>(UINT_MAX));
	}
	// -------------------------------------------------------------------------
	void LCGenerator::srand_impl(uint32 seed)
	{
		this->seed = seed;
		//srand(seed);
	}
	// -------------------------------------------------------------------------
	uint32 LCGenerator::rand_impl()
	{
		return (((seed = seed * _a + _c) >> 16) & 0x7fff);
		//return rand();
	}
	// -------------------------------------------------------------------------
	void MTGenerator::srand_impl(uint32 seed)
	{
		register uint32* s = state; // wskaznik na tablice
		register uint32* r = state; // wskaznik o element wczesniej

		*s++ = seed & 0xffffffffUL;

		for(int i = 1 ; i < N; ++i)
		{
			*s++ = (1812433253UL * (*r ^ (*r >> 30)) + i) & 0xffffffffUL;
			r++;
		}
	}
	// -------------------------------------------------------------------------
	void MTGenerator::reload(void)
	{
		for(int i = 0; i < N; ++i)
		{
			int y = ((state[i] & 0x8000000) + (state[(i+1) % N])) & 0x7fffffffUL;
			state[i] = state[(i + M) % N] ^ (y >> 1);

			if(y % 2) // y jest nieparzysty
				state[i] ^= 0x9908b0df;
		}
	}
	// -------------------------------------------------------------------------
	uint32 MTGenerator::rand_impl()
	{
		if(idx == 0)
			reload();
		idx = (idx + 1) % N;

		register uint32 y = state[idx-1];
		y ^= (y >> 11);
		y ^= (y << 7) & 0x9d2c5680UL;
		y ^= (y << 15) & 0xefc60000UL;
		return (y ^ (y >> 18));
	}
	// -------------------------------------------------------------------------
	inline void ISAACGenerator::shuffle(uint32& a, uint32& b, uint32& c, uint32& d,
		uint32& e, uint32& f, uint32& g, uint32& h)
	{
		a ^= b<<11; d += a; b += c;
		b ^= c>>2;  e += b; c += d;
		c ^= d<<8;  f += c; d += e;
		d ^= e>>16; g += d; e += f;
		e ^= f<<10; h += e; f += g;
		f ^= g>>4;  a += f; g += h;
		g ^= h<<8;  b += g; h += a;
		h ^= a>>9;  c += h; a += b;
	}
	// -------------------------------------------------------------------------
	inline uint32 ISAACGenerator::ind(uint32* mm, uint32 x)
	{
		return (*(uint32*)((unsigned char*)(mm) + ((x) & ((N - 1) << 2))));
	}
	// -------------------------------------------------------------------------
	inline void ISAACGenerator::rngstep(uint32 mix, uint32& a, uint32& b, uint32*& mm,
		uint32*& m, uint32*& m2, uint32*& r, uint32& x, uint32& y)
	{
		x = *m;
		a = (a^(mix)) + *(m2++);
		*(m++) = y = ind(mm, x) + a + b;
		*(r++) = b = ind(mm, y >> RANDSIZL) + x;
	}
	// -------------------------------------------------------------------------
	void ISAACGenerator::srand_impl(uint32 seed)
	{
		ctx.randa = seed & 0x7fff;
		ctx.randb = (seed << 8) & 0x7fff;;
		ctx.randc = (seed >> 8) & 0x7fff;
		randinit();
	}
	// -------------------------------------------------------------------------
	void ISAACGenerator::srand_impl(uint32 a, uint32 b, uint32 c, uint32* s)
	{
		for(int i = 0; i < N; ++i)
			ctx.randrsl[i] = s != 0 ? s[i] : 0;

		ctx.randa = a;
		ctx.randb = b;
		ctx.randc = c;

		randinit();
	}
	// -------------------------------------------------------------------------
	uint32 ISAACGenerator::rand_impl()
	{
		ctx.randcnt--;

		if(!ctx.randcnt)
		{
			isaac();
			ctx.randcnt = N - 1;
		}
		return ctx.randrsl[ctx.randcnt];
	}
	// -------------------------------------------------------------------------
	void ISAACGenerator::randinit(void)
	{
		uint32 a, b, c, d, e, f, g, h;
		a = b = c = d = e = f = g = h = 0x9e3779b9UL; // the golden ratio

		uint32* m = ctx.randmem;
		uint32* r = ctx.randrsl;

		// scramble it
		for(int i = 0; i < 4; ++i)
			shuffle(a, b, c, d, e, f, g, h);

		// initialise using the contents of r[] as the seed
		for(int i = 0; i < N; i += 8)
		{
			a += r[i  ]; b += r[i+1]; c += r[i+2]; d += r[i+3];
			e += r[i+4]; f += r[i+5]; g += r[i+6]; h += r[i+7];

			shuffle(a, b, c, d, e, f, g, h);

			m[i  ] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
			m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
		}

		//do a second pass to make all of the seed affect all of m
		for(int i = 0; i < N; i += 8)
		{
			a += m[i  ]; b += m[i+1]; c += m[i+2]; d += m[i+3];
			e += m[i+4]; f += m[i+5]; g += m[i+6]; h += m[i+7];

			shuffle(a,b,c,d,e,f,g,h);

			m[i  ] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
			m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
		}

		isaac();
		ctx.randcnt = N;
	}
	// -------------------------------------------------------------------------
	void ISAACGenerator::isaac(void)
	{
		register uint32 x, y;

		register uint32* mm = ctx.randmem;
		register uint32* r= ctx.randrsl;
		register uint32 a = ctx.randa;
		register uint32 b = ctx.randb + (++ctx.randc);
		register uint32* m = mm;
		register uint32* m2 = m + (N / 2);
		register uint32* mend = m2;

		for (; m < mend; )
		{
			rngstep(a<<13, a, b, mm, m, m2, r, x, y);
			rngstep(a>>6 , a, b, mm, m, m2, r, x, y);
			rngstep(a<<2 , a, b, mm, m, m2, r, x, y);
			rngstep(a>>16, a, b, mm, m, m2, r, x, y);
		}
		for (m2 = mm; m2 < mend; )
		{
			rngstep(a<<13, a, b, mm, m, m2, r, x, y);
			rngstep(a>>6 , a, b, mm, m, m2, r, x, y);
			rngstep(a<<2 , a, b, mm, m, m2, r, x, y);
			rngstep(a>>16, a, b, mm, m, m2, r, x, y);
		}

		ctx.randa = a;
		ctx.randb = b;
	}
	// -------------------------------------------------------------------------
}

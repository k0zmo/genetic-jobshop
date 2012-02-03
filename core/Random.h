#include "Prerequisites.h"

namespace core
{
	// forward decl
	class Random_pimpl;

	class Random
	{
	public:
		Random();
		~Random();

		enum EGeneratorType
		{
			GT_LINEAR,
			GT_MERSENNE_TWISTER,
			GT_ISAAC
		};

		void setGenerator(EGeneratorType type);
		void srand(uint32 seed);
		void setBound(uint32 a, uint32 b);
		
		// zwraca liczbe z przedzialu okreslonego przez funkcje setBound
		uint32 random();
		
		// zwraca liczbe z przedzialu [a,b]
		uint32 random(uint32 a, uint32 b);
		
		// zwraca liczbe z przedzialu [0.0f; 1.0f]
		float randomUnorm();
		
		uint32 getLowerBound() const { return a; }
		uint32 getUpperBound() const { return b; }
	private:
		Random_pimpl* mpGenerator;
		uint32 a, b;
	};
}

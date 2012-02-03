#include <omp.h>

#include "core/Random.h"

// * Gen: Dla kazdej operacji przypada jeden taki opisujacy na ktorej maszynie
//        operacja zostanie wykonana i w jakim czasie zostanie rozpoczeta
struct Gene
{
	core::uint32 machine;
	core::uint32 time;

	Gene() {}
	Gene(core::uint32 machine, core::uint32 time)
		: machine(machine), time(time){}
};

// * Zbior genow (wiersz) koduje rozwiazanie dla jednego zadania
struct JobGene
{
	std::vector<Gene> genes;
};

class Problem;

// * Jedno z rozwiazan
struct Genome
{
	Genome(Problem& p);

	// Zbior wierszy genow = tabelka = jedno z rozwiazan
	std::vector<JobGene> rows;
	core::uint32 objective;
	float fitness; // przystosowanie (im mniejsze typ lepsze) - przeskalowane objective (np. dla ruletki)
};

//////////////////////////////////////////////////////////////////////////

// * Operacja: wektor kosztow dla kazdej maszyny
struct Operation
{
	std::vector<int> costs;
};

// * Zadanie: Zbior operacji od siebie zaleznych,
//            kazde z zadan jest od siebie niezalezne
struct Job
{
	std::vector<Operation> ops;
	core::uint32 numOps;
};

class QString;

// * Wczytany problem do rozwiazania.
class Problem
{
	friend struct Genome;
public:
	Problem();
	~Problem();

	enum ESelectionScheme
	{
		// Stochastic uniform selection picks randomly from the 
		// population.  Each individual has as much chance as any other.
		SS_UNIFORM,
		// Weighted selection where individuals with better fitness have
		// a greater chance of being selected than those with lower 
		// scores.
		SS_ROULETTE,
		// Similar to roulette, but instead of choosing one, choose two
		// then pick the better of the two as the selected individuals
		SS_TOURNAMENT,
		// Stochastic universal sampling does a preselection based on 
		// the expected number of each genome, then a random sampling on 
		// the preselected list.
		SS_SUS,
		// Pick the genome with the best fitness (not objective score)
		SS_RANKING,
	};

	enum EPopulationModel
	{
		PM_SIMPLE,
		PM_SS_UNIFORM,
		PM_SS_ELITISM,
		PM_SS_EXCESS
	};

	enum EFitnessModel
	{
		FM_LINEARRANKING,
		FM_SIGMASCALING,
		FM_RAWVALUE
	};

	void clearData();
	void clearAllSolutions();
	bool loadInitialData(const char* filename);
	void generateRandomSolutions(core::uint32 populationSize);
	void showPopulation();
	void showPopulationStats();
	#if defined(EVO_QT_SUPPORT)
	void getPopulationDesc(QString& desc);
	#endif

	// Ustawia prawdopodobienstwo krzyzowaia i mutacji nowych osobnikow
	void setProbability(float probCrossover, float probMutation)
	{
		probCX = std::max(std::min(probCrossover, 1.0f), 0.0f);
		probMUT = std::max(std::min(probMutation, 1.0f), 0.0f);;
	}

	// Ustawia metode selekcji
	void setSelectMethod(ESelectionScheme ss)
	{
		ssMethod = ss;
		switch(ssMethod)
		{
		case SS_UNIFORM: pfnSelect = &Problem::selectUniform; break;
		case SS_ROULETTE: pfnSelect = &Problem::selectRoulette; break;
		case SS_RANKING: pfnSelect = &Problem::selectRanking; break;
		case SS_TOURNAMENT: pfnSelect = &Problem::selectTournament; break;
		case SS_SUS: pfnSelect = &Problem::selectSus; break;
		default: break;
		}
	}

	void setPopulationModel(EPopulationModel pm)
	{
		popModel = pm;
		switch(popModel)
		{
		case PM_SIMPLE: pfnNextGen = &Problem::nextGenSimple; break;
		case PM_SS_ELITISM:
		case PM_SS_EXCESS:
		case PM_SS_UNIFORM:
			pfnNextGen = &Problem::nextGenSteadyState; break;
		default: break;
		}
	}

	// Ile osobnikow generowac (czyli rowniez zastepowac) dla populacji z modelem steady state
	void setSSParameters(core::uint32 TempPopSize, float ReplaceCoeff, core::uint32 popSize)
	{
		// maja rozne zbiory dopuszczalne
		// Dla PM_SS_ELITISM:
		// * TempPopSize <= pop.size()
		// * ReplaceCoeff unused
		// Dla PM_SS_UNIFORM:
		// * TempPopSize <= pop.size()
		// * ReplaceCoeff unused
		// Dla PM_SS_EXCESS:
		// * TempPopSize >= pop.size()
		// * ReplaceCoeff in [0.0f, 1.0f]

		switch(popModel)
		{
		case PM_SS_ELITISM: 
		case PM_SS_UNIFORM:
			tempPopSize = std::max(std::min(TempPopSize, popSize), 1U);
			break;
		case PM_SS_EXCESS:
			tempPopSize = std::max(TempPopSize, popSize+1);
			break;
		}
		replaceCoeff = std::max(std::min(ReplaceCoeff, 1.0f), 0.0f);
	}

	void setGenitor(bool enable)
	{
		genitor = enable;
	}

	// Wybor sposobu obliczania fitness score'a
	void setFitnessModel(EFitnessModel fm, float selectivePressure)
	{
		fitModel = fm;
		sp = std::max(std::min(selectivePressure, 2.0f), 1.0f);
	}

	// Prawdopodobiestwo wyboru operatora krzyzowania COLUMN
	// Dla 1.0f wybierany bedzie tylko operator COLUMN
	// Dla 0.0f wybierany bedzie tylko operator ROW
	// Dla posrednich bedzie to losowe, zalezne od parametru 'prob'
	void setOperatorProbability(float prob)
	{ 
		probOperator = std::max(std::min(prob, 1.0f), 0.0f);
	}

	void setTournamentParameters(core::uint32 groupSize, bool allowDuplicates)
	{
		tourGroupSize = groupSize;
		pickUnused = allowDuplicates;
	}

	// Nastepne pokolenie
	void nextGen();

	// Wynik wypisz do matlab'a
	bool outputToMatlab(const char* filename, core::uint32 genomeIndex = 0);
	bool outputToMatlab(const char* filename, Genome& gen);

private:
	std::vector<Job> jobs;

	typedef std::vector<Genome*> Population;
	Population pop;
	Population tmpPop;

	int indexPop;
	float replaceCoeff;
	core::uint32 tempPopSize;

	core::uint32 numMachines;
	core::uint32 numJobs;
	core::uint32 maxOps;
	core::Random rnd;

	ESelectionScheme ssMethod;
	EPopulationModel popModel;
	EFitnessModel fitModel;
	bool genitor;
	float sp; // selective pressure
	float probCX;
	float probMUT;
	// prawdopodobienstwo wyboru jednego z dwoch operator krzyzowania
	float probOperator;
	core::uint32 tourGroupSize;
	bool pickUnused;

	// Wektor losowych kolorow dla wykresy Gantt'a
	struct rgb { float r, g, b; };
	std::vector<rgb> colors;

	// Liczy wartosc funkcji celu dla pojedynczego rozwiazania
	core::uint32 objectiveScore(Genome& gen);
	// Oblicza wsp. przystosowania kazdego z osobnikow z populacji (przeskalowana wartosc funkcji celu)
	void fitness(Population& pop);

	// Liczy czas rozpoczecia procesu na maszynach z uwzglednieniem zaleznosci i zajecia maszyn
	core::uint32 computeStartingTime(Genome& gen);

	// Oblicza statystyki dla populacji (srednia, min, max, odchylenie)
	void calcStats(Population& pop);
	// Zwraca indeks najlepszego osobnika
	core::uint32 best(Population& pop);
	// Zwraca indeks najgorszego osobnika
	core::uint32 worst(Population& pop);

	// wskaznik na odpowiednia metode selekcji
	Genome& (Problem::*pfnSelect)(Population& pop);
	// wskanzik na odpowiednia metode generowania nowego pokolenia (w zaleznosci od modelu populacji)
	void (Problem::*pfnNextGen)(Population& popSrc, Population& popDst);

	// Operator mutacji
	void mutate(const Genome& in, Genome& out);

	// Operatory krzyzowania
	void columnCrossover(const Genome& mom, const Genome& dad,
		Genome& kid1, Genome& kid2);
	void rowCrossover(const Genome& mom, const Genome& dad,
		Genome& kid1, Genome& kid2);

	// Metody selekcji
	Genome& selectUniform(Population& pop);
	Genome& selectRoulette(Population& pop);
	Genome& selectTournament(Population& pop);
	Genome& selectRanking(Population& pop);
	Genome& selectSus(Population& pop);

	void preselectRoulette(Population& pop); // rowniez dla metody turniejowej
	void preselectSus(Population& pop);

	// Metody generowania nowego pokolenia
	void create1New(int i, Population& popSrc, Population& popDst);
	void create2New(int i, Population& popSrc, Population& popDst);
	void nextGenSimple(Population& popSrc, Population& popDst);
	void nextGenSteadyState(Population& popSrc, Population& popDst);

	void sort(Population& pop);
	float* psum;
	core::uint32* choices;

public:
	// Statystyki
	float average;
	float averageInv;
	float stdDeviation;
	float stdDeviationInv;
	core::uint32 maxObjective;
	core::uint32 minObjective;
	float averageFitness;
	float maxFitness;
	float minFitness;
};

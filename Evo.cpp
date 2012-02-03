#include <algorithm>
#include <cmath>
#include <set>

#if defined(EVO_QT_SUPPORT)
#include <QString>
#include <QTextStream>
#endif

#include "Evo.h"

using namespace core;

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	ifndef NOMINMAX
#	define NOMINMAX
#	endif
#	include <windows.h>
#	ifdef _MSC_VER
#		pragma warning(disable : 4018)
#	endif
#else
#	include <sys/time.h>
#endif

Genome::Genome(Problem& p)
: objective(0xFFFFFFFF)
{
	rows.resize(p.numJobs);

	for(uint32 i = 0; i < p.numJobs; ++i)
	{
		rows[i].genes.resize(p.jobs[i].numOps);
	}
}

// zwraca indeks najwiekszego elementu w tablicy
template<typename T>
uint32 getMaxIdx(T* arr, uint32 arraysize)
{
	T maxVal = arr[0];
	uint32 maxId = 0;

	for(uint32 i = 0; i < arraysize; ++i)
	{
		if(arr[i] > maxVal)
		{
			maxVal = arr[i];
			maxId = i;
		}
	}
	return maxId;
}

// Zwraca indeks najmniejszego elementu w tablicy
uint32 getMinIdx(uint32* arr, uint32 arraysize)
{
	uint32 minVal = arr[0];
	uint32 minId = 0;

	for(uint32 i = 0; i < arraysize; ++i)
	{
		if(arr[i] < minVal)
		{
			minVal = arr[i];
			minId = i;
		}
	}
	return minId;
}

// -------------------------------------------------------------------------
Problem::Problem()
: jobs(0), indexPop(0), replaceCoeff(0.1f), tempPopSize(0), numMachines(0), numJobs(0),
maxOps(0), popModel(PM_SIMPLE), fitModel(FM_LINEARRANKING), genitor(true),
sp(2.0f), probCX(0.0f), probMUT(0.0f),probOperator(0.5f), tourGroupSize(4), pickUnused(true),
average(0.0f), maxObjective(0), minObjective(0),
averageFitness(0.0f), maxFitness(0.0f), minFitness(0.0f)
{
#ifdef _WIN32
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

	rnd.setGenerator(Random::GT_MERSENNE_TWISTER);
	rnd.srand(static_cast<uint32>((time.QuadPart & 0x0000FFFFFFFF0000LL) >> 4));
#else
	
	timeval curr;
	gettimeofday(&curr, NULL);

	rnd.setGenerator(Random::GT_MERSENNE_TWISTER);
	rnd.srand(static_cast<uint32>(curr.tv_usec));
#endif

	pfnSelect = &Problem::selectUniform;
	pfnNextGen = &Problem::nextGenSimple;
}
// -------------------------------------------------------------------------
Problem::~Problem()
{
	clearData();
}
// -------------------------------------------------------------------------
void Problem::clearData()
{
	colors.clear();
	jobs.clear();

	clearAllSolutions();
}
// -------------------------------------------------------------------------
void Problem::clearAllSolutions()
{
	for(Population::iterator it = pop.begin();
		it != pop.end(); ++it)
	{
		delete *it;
	}

	for(Population::iterator it = tmpPop.begin();
		it != tmpPop.end(); ++it)
	{
		delete *it;
	}

	pop.clear();
	tmpPop.clear();

	delete [] psum;
	delete [] choices;
}
// -------------------------------------------------------------------------
bool Problem::loadInitialData(const char* filename)
{
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if(!file.is_open())
	{
		fprintf(stderr, "Error: couldn't open \"%s\"\n", filename);
		return false;
	}

	char buff[512];
	int magic;
	int currJob = 0;

	while(!file.eof())
	{
		// przeczytaj cala linie
		file.getline(buff, 512);

		if(!strcmp(buff, "\r") && !strcmp(buff, "\n"))
			continue;

		if(sscanf(buff, "magic %d", &magic) == 1)
		{
			if(magic != 1337)
			{
				// bad version
				fprintf(stderr, "Error: bad file format.\n");
				file.close();
				return false;
			}
		}

		else if(sscanf(buff, "numJobs %d", &numJobs) == 1)
		{
			if(numJobs)
			{
				jobs.resize(numJobs);
				// zeby sie valgrind nie czepial
				for(uint32 i = 0; i < numJobs; ++i)
					jobs[i].numOps = 0;
			}
		}

		else if(sscanf(buff, "numMachines %d", &numMachines) == 1)
		{
		}

		else if(!strncmp(buff, "job {", 5))
		{
			Job* job = &jobs[currJob];
			uint32 currentOps = 0;

			while((buff[0] != '}') && !file.eof())
			{
				// przeczytaj nast. linijke
				file.getline(buff, 512);

				if(sscanf(buff, "\tnumOperations %d", &job->numOps) == 1)
				{
					if(job->numOps > 0)
					{
						job->ops.resize(job->numOps);

						for(uint32 k = 0; k < job->numOps; ++k)
						{
							job->ops[k].costs.resize(numMachines);
						}
					}
				}

				else if(!strncmp(buff, "}", 1) == 1)
					break;

				else
				{
					char* tmpBuf = buff;
					for(uint32 k = 0; k < job->ops[currentOps].costs.size(); ++k)
					{
						sscanf(tmpBuf, "%d", &job->ops[currentOps].costs[k]);

						char convBuf[32];
						sprintf(convBuf, "%d", job->ops[currentOps].costs[k]);
						tmpBuf += strlen(convBuf) + 1;
					}

					currentOps++;
				}
			}

			currJob++;
		}
	}

	// maksymalna ilosc operacji
	for(uint32 i = 0; i < numJobs; ++i)
		maxOps = std::max(jobs[i].numOps, maxOps);

	// wygeneruj mape kolorow
	colors.resize(numJobs);
	for(uint32 i = 0; i < numJobs; ++i)
	{		
		colors[i].r = rnd.randomUnorm();
		colors[i].g = rnd.randomUnorm();
		colors[i].b = rnd.randomUnorm();
	}

	file.close();
	return true;
}
// -------------------------------------------------------------------------
void Problem::generateRandomSolutions(uint32 populationSize)
{
	rnd.setBound(0, numMachines - 1);

	size_t tmpPopSize;

	if(popModel != PM_SIMPLE)
		tmpPopSize = tempPopSize;
	else
		tmpPopSize = populationSize;

	tmpPop.reserve(tmpPopSize);
	pop.reserve(populationSize);

	for(uint32 i = 0; i < populationSize; ++i)
	{
		Genome* newGenome = new Genome(*this);

		for(uint32 k = 0; k < numJobs; ++k)
		{
			for(uint32 j = 0; j < jobs[k].ops.size(); ++j)
			{
				uint32 machine = rnd.random();
				newGenome->rows[k].genes[j] = Gene(machine, -1);
			}
		}

		computeStartingTime(*newGenome);
		pop.push_back(newGenome);
	}

	for(uint32 i = 0; i < tmpPopSize; ++i)
	{
		// tylko aby zaalokowac pamiec i nie robic tego w kazdym kroku (defragmentacja)
		Genome* tmpGenome = new Genome(*this);
		tmpPop.push_back(tmpGenome);
	}

	//printf("size of tmpPop: %d\nsize of pop: %d\n", tmpPop.size(), pop.size());

	sort(pop);
	calcStats(pop);
	fitness(pop);

	// Wartosci (suma prawdopodobienstwa) dla ruletki
	psum = new float[pop.size()];
	choices = new uint32[pop.size()];
}
// -------------------------------------------------------------------------
void Problem::showPopulation()
{
	Population& popSrc = (indexPop ? tmpPop : pop);

	printf("\n======= Population: =======\n");

	for(size_t i = 0; i < popSrc.size(); ++i)
		printf("%d ", popSrc[i]->objective);

	printf("\n");
}
// -------------------------------------------------------------------------
void Problem::showPopulationStats()
{
	printf("Min value of objective function: %d\n", minObjective);
	printf("Max value of objective function: %d\n", maxObjective);
	printf("Avg value of objective function: %f\n", average);
}
// -------------------------------------------------------------------------
#if defined(EVO_QT_SUPPORT)
void Problem::getPopulationDesc(QString& desc)
{
	Population& popSrc = (indexPop ? tmpPop : pop);
	QTextStream strm(&desc);

	strm << "======= Population: =======\n";
	for(size_t i = 0; i < popSrc.size(); ++i)
		strm << popSrc[i]->objective << " ";
	strm << "\n";
	strm << "Min value of objective function: " << minObjective << "\n";
	strm << "Max value of objective function: " << maxObjective << "\n";
	strm << "Avg value of objective function: " << average << "\n";
	strm << "Std deviation of objective function: " << stdDeviation << "\n";
}
#endif
// -------------------------------------------------------------------------
bool sortPredicate(Genome* a, Genome* b)
{
	return a->objective < b->objective;
}
// -------------------------------------------------------------------------
void Problem::sort(Population& pop)
{
	std::sort(pop.begin(), pop.end(), sortPredicate);
}
// -------------------------------------------------------------------------
void Problem::fitness(Population& pop)
{
	// * Sigma scaling
	if(fitModel == FM_SIGMASCALING)
	{
		#pragma omp parallel for
		for(int32 i = 0; i < static_cast<int32>(pop.size()); ++i)
		{
			// w locie - zamiana minimalizacji na maksymalizacje
			// (amerykanska literatura)
			pop[i]->fitness = std::max(static_cast<float>((maxObjective - pop[i]->objective))
				- (averageInv - sp * stdDeviationInv), 0.0f);
		}
	}
	// * Linear ranking
	else if(fitModel == FM_LINEARRANKING)
	{
		#pragma omp parallel for
		for(int32 i = 0; i < static_cast<int32>(pop.size()); ++i)
		{
			// pos is the position of an individual in this population
			// (least fit individual has Pos=1, the fittest individual Pos=Nind)
			float pos = static_cast<float>(pop.size() - i);
			pop[i]->fitness = 2.0f - sp + 2.0f * (sp - 1.0f)
				* (pos - 1.0f) / static_cast<float>(pop.size() - 1);
		}
	}
	else if(fitModel == FM_RAWVALUE)
	{
		#pragma omp parallel for
		for(int32 i = 0; i < static_cast<int32>(pop.size()); ++i)
		{
			// map 1:1
			pop[i]->fitness = static_cast<float>(maxObjective - pop[i]->objective);
		}
	}

	// Statystyki dla fitness score'a

	maxFitness = pop[0]->fitness;
	minFitness = pop[0]->fitness;
	averageFitness = 0.0f;

	for(Population::iterator it = pop.begin(); it != pop.end(); ++it)
	{
		const float& f = (*it)->fitness;
		maxFitness = std::max(f, maxFitness);
		minFitness = std::min(f, minFitness);
		averageFitness += f;
	}
	averageFitness /= static_cast<float>(pop.size());
}
// -------------------------------------------------------------------------
void Problem::calcStats(Population& pop)
{
	maxObjective = pop[0]->objective;
	minObjective = pop[0]->objective;
	average = 0.0f;
	averageInv = 0.0f;

	for(uint32 i = 0; i < pop.size(); ++i)
	{
		uint32 f = pop[i]->objective;
		minObjective = std::min(f, minObjective);
		maxObjective = std::max(f, maxObjective);
		average += f;
	}
	average /= pop.size();

	for(uint32 i = 0; i < pop.size(); ++i)
	{
		uint32 f = maxObjective - pop[i]->objective;
		averageInv += f;
	}
	averageInv /= pop.size();

	// Odchylenie standardowe
	stdDeviation = 0.0f;
	stdDeviationInv = 0.0f;
	for(uint32 i = 0; i < pop.size(); ++i)
	{
		float f = static_cast<float>((maxObjective - pop[i]->objective) - averageInv);
		stdDeviationInv += f * f;

		f = static_cast<float>(pop[i]->objective - average);
		stdDeviation += f * f;
	}
	stdDeviation = sqrtf(stdDeviation / pop.size());
	stdDeviationInv = sqrtf(stdDeviationInv / pop.size());
}
// -------------------------------------------------------------------------
uint32 Problem::best(Population& pop)
{
	uint32 index = 0;
	uint32 bestVal = pop[index]->objective;
	for(uint32 i = 0; i < pop.size(); ++i)
	{
		if(pop[i]->objective < bestVal)
			index = i;
	}
	return index;
}
// -------------------------------------------------------------------------
uint32 Problem::worst(Population& pop)
{
	uint32 index = 0;
	uint32 worstVal = pop[index]->objective;
	for(uint32 i = 0; i < pop.size(); ++i)
	{
		if(pop[i]->objective > worstVal)
			index = i;
	}
	return index;
}
// -------------------------------------------------------------------------
void Problem::preselectRoulette(Population& pop)
{
	uint32 n = pop.size();

	if(minObjective == maxObjective)
	{
		for(uint32 i = 0; i < n; ++i)
			psum[i] = (float)(i + 1)/(float)n; // equal likelihoods
	}
	else
	{
		// populacja popSrc jest posortowana wg fitness'a
		psum[0]= -pop[0]->fitness;

		for(uint32 i = 1; i < n; i++)
			psum[i] = -pop[i]->fitness + psum[i-1];
		for(uint32 i = 0; i < n; i++)
			psum[i] /= psum[n-1];
	}
}
// -------------------------------------------------------------------------
void Problem::preselectSus(Population& pop)
{
	uint32 n = pop.size();

	if(averageFitness == 0 || maxFitness == minFitness)
	{
		for(uint32 i = 0; i < n; ++i)
			choices[i] = rnd.random(0, n - 1);
	}
	else
	{
		int k = 0;
		#define fraction psum
		for(uint32 i = 0; i < n; ++i)
		{
			float expected = pop[i]->fitness / averageFitness;
			int ne = static_cast<int>(expected);
			fraction[i] = expected - ne;

			while(ne > 0 && k < static_cast<int>(n))
			{
				assert(k >= 0 && k < static_cast<int>(n));
				choices[k] = i;
				++k;
				--ne;
			}
		}

		int i = 0;
		int flag = 0;

		while(k < static_cast<int>(pop.size()) && flag)
		{
			if(i >= static_cast<int>(pop.size()))
			{
				i = 0;
				flag = 0;
			}

			if(fraction[i] > 0.0f && rnd.randomUnorm() > 0.5f)
			{
				assert(k >= 0 && k < static_cast<int>(n));
				assert(i >= 0 && i < static_cast<int>(n));
				choices[k] = i;
				fraction[i] -= 1.0;
				++k;
				flag = 1;
			}
			++i;
		}

		if(k < static_cast<int>(pop.size()))
		{
			for(; k < static_cast<int>(pop.size()); ++k)
			{
				choices[k] = rnd.random(0, pop.size() - 1);
			}
		}
		#undef fraction
	}
}
// -------------------------------------------------------------------------
Genome& Problem::selectUniform(Population& pop)
{
	// * Random Selection
	// Randomly select an individual from the population.  This selector does not
	// care whether it operates on the fitness or objective scores.
	return *pop[rnd.random(0, pop.size() - 1)];
}
// -------------------------------------------------------------------------
Genome& Problem::selectRoulette(Population& pop)
{
	// * Roulette Wheel Selection
	// We look through the members of the population using a weighted roulette wheel.
	// Likliehood of selection is proportionate to the fitness score.
	float cutoff = rnd.randomUnorm();
	int lower = 0;
	int upper = pop.size() - 1;

	// binary search
	while(upper >= lower)
	{
		int i = lower + (upper - lower) / 2;
		assert(i >= 0 && i < (int)pop.size());

		if(psum[i] > cutoff)
			upper = i - 1;
		else
			lower = i + 1;
	}

	lower = std::min((int)pop.size() - 1, lower);
	lower = std::max(0, lower);

	return *pop[lower];
}
// -------------------------------------------------------------------------
Genome& Problem::selectTournament(Population& pop)
{
	// * Tournament Selection
	// Pick two or more random individuals from the population and select the best of them
	uint32 tSize = tourGroupSize;
	std::vector<uint32> tGroup; tGroup.reserve(tSize);

	assert(tSize >= 2 && tSize <= pop.size());

	if(pickUnused)
	{
		for(uint32 i = 0; i < tSize; ++i)
		{
			uint32 rand;
			std::vector<uint32>::iterator result;

			// pick up random individual from the population
			// (must be unused)
			do
			{
				rand = rnd.random(0, pop.size() - 1);
				result = find(tGroup.begin(), tGroup.end(), rand);
			}
			while(result != tGroup.end());

			// add to tournament group
			tGroup.push_back(rand);
		}
	}

	else
	{
		for(uint32 i = 0; i < tSize; ++i)
		{
			uint32 rand = rnd.random(0, pop.size() - 1);
			// add to tournament group
			tGroup.push_back(rand);
		}
	}

	uint32 best = tGroup[0];
	// select best individual from tournament group
	for(uint32 i = 1; i < tSize; ++i)
	{
		if(pop[tGroup[i]]->fitness > pop[best]->fitness)
		{
			best = tGroup[i];
		}
	}

	return *pop[best];
}
// -------------------------------------------------------------------------
Genome& Problem::selectRanking(Population& pop)
{
	// * Ranking Selection
	// Any population may contain more than one individual with the same score.
	// This method must be able to return any one of those 'best' individuals, so
	// we do a short search here to find out how many of those 'best' there are.
	// This routine assumes that the 'best' individual is that with index 0.
	uint32 bound = 0;
	while((bound < pop.size() - 1) && (pop[++bound]->objective == pop[0]->objective));
	--bound;

	if(bound == 0)
		return *pop[0];
	else
		return *pop[rnd.random(0, bound)];
}
// -------------------------------------------------------------------------
Genome& Problem::selectSus(Population& pop)
{
	// * Stochastic remainder sampling
	// The selection happens in two stages.  First we generate an array using the
	// integer and remainder parts of the expected number of individuals.  Then we
	// pick an individual from the population by randomly picking from this array.

	// This is implemented just as in Goldberg's book.  Not very efficient...  In
	// Goldberg's implementation he uses a variable called 'nremain' so that multiple
	// calls to the selection routine can be dependent upon previous calls.  We don't
	// have that option with this architecture; we would need to make selection an
	// object coupled closely with the population to make that work.

	return *pop[choices[rnd.random(0, pop.size() - 1)]];
}
// -------------------------------------------------------------------------
void Problem::create1New(int i, Population& popSrc, Population& popDst)
{
	Genome& mom = (this->*(pfnSelect))(popSrc);
	Genome& dad = (this->*(pfnSelect))(popSrc);

	// Czy krzyzujemy
	float pcx = rnd.randomUnorm();
	if(pcx <= probCX)
	{
		Genome tmp(*this);

		if(rnd.randomUnorm() > probOperator)
			rowCrossover(mom, dad, *popDst[i], tmp);
		else
			columnCrossover(mom, dad, *popDst[i], tmp);
	}
	else
	{
		if(rnd.randomUnorm() > 0.5f)
			*popDst[i] = dad;
		else
			*popDst[i] = mom;
	}

	// Czy mutujemy
	float pmut = rnd.randomUnorm();
	if(pmut <= probMUT)
		mutate(*popDst[i], *popDst[i]);
}
// -------------------------------------------------------------------------
void Problem::create2New(int i, Population& popSrc, Population& popDst)
{
	Genome& mom = (this->*(pfnSelect))(popSrc);
	Genome& dad = (this->*(pfnSelect))(popSrc);

	// Czy krzyzujemy
	float pcx = rnd.randomUnorm();
	if(pcx <= probCX)
	{
		if(rnd.randomUnorm() > probOperator)
			rowCrossover(mom, dad, *popDst[i], *popDst[i+1]);
		else
			columnCrossover(mom, dad, *popDst[i], *popDst[i+1]);
	}
	else
	{
		// przenies rodzicow do nastepnego pokolenia
		*popDst[i] = mom;
		*popDst[i+1] = dad;
	}

	// Czy mutujemy
	float pmut = rnd.randomUnorm();
	if(pmut <= probMUT)
		mutate(*popDst[i], *popDst[i]);
	pmut = rnd.randomUnorm();
	if(pmut <= probMUT)
		mutate(*popDst[i+1], *popDst[i+1]);
}
// -------------------------------------------------------------------------
void Problem::nextGenSteadyState(Population& popSrc, Population& popDst)
{
	//assert(popOverlap <= popSrc.size());

	// popSrc sie w wiekszosci nie zmienia
	// popDst jest to populacja tymczasowa

	for(uint32 i = 0; i < popDst.size() - 1; i += 2)
		create2New(i, popSrc, popDst);

	if(popDst.size() % 2 != 0)
		create1New(popDst.size() - 1, popSrc, popDst);

	// * 4+3
	if(popModel == PM_SS_EXCESS)
	{
		sort(popDst);

		replaceCoeff = std::min(std::max(replaceCoeff, 0.0f), 1.0f);

		uint32 nReplace = static_cast<uint32>(floorf(replaceCoeff * popSrc.size()));
		uint32 i = popSrc.size() - nReplace;
		uint32 j = 0;

		for(; i < popSrc.size(); ++i)
		{
			if(popSrc[i]->objective > popDst[j]->objective)
			{
				*popSrc[i] = *popDst[j];
				j++;
			}
		}
	}

	// * ELITISM REINSERTION
	else if(popModel == PM_SS_ELITISM)
	{
		sort(popDst);

		// Przyklad:
		// popSrc = [3 4 5 6 9], iterujemy po i=popSrc.size()-popDst.size()=2
		// popDst = [2 8 10], iterujemy po j=0
		// 1* porownujemy 2 z 5, jest lepsze, zastepujemy, i++, j++
		// 2* porownujemy 8 z 6, jest gorsze, i++, j zostaje
		// 3* porownujemy 8 z 9, jest lepsze, zastepujemy, i++, j++
		// koniec po i

		uint32 i = popSrc.size() - popDst.size();
		uint32 j = 0;

		for(; i < popSrc.size(); ++i)
		{
			if(popSrc[i]->objective > popDst[j]->objective)
			{
				*popSrc[i] = *popDst[j];
				j++;
			}
		}
	}
	// * UNIFORM REINSERTION
	else if(popModel == PM_SS_UNIFORM)
	{
		std::set<uint32> replaced;
		for(uint32 j = 0; j < popDst.size(); ++j)
		{
			uint32 choice;
			do
			{
				choice = rnd.random(0, popSrc.size() - 1);
			} while(replaced.find(choice) != replaced.end());
			replaced.insert(choice);

			*popSrc[choice] = *popDst[j];
		}
	}

	//printf("\n======= POP: =======\n");
	//for(Population::iterator it = popSrc.begin();
	//	it != popSrc.end(); ++it)
	//	printf("%d ", (*it)->objective);
	//printf("\n\n");

	// Aktualizuj dane dla populacji
	sort(popSrc);
	calcStats(popSrc);
	fitness(popSrc);
}
// -------------------------------------------------------------------------
void Problem::nextGenSimple(Population& popSrc, Population& popDst)
{
	#pragma omp parallel for
	for(int32 i = 0; i < static_cast<int32>(pop.size() - 1); i += 2)
		create2New(i, popSrc, popDst);

	if(popSrc.size() % 2 != 0)
		create1New(popSrc.size() - 1, popSrc, popDst);
	
	// 1->0->1->0
	++indexPop;
	indexPop = indexPop % 2;

	// * GENITOR
	// Przenies najlepszego osobnika ze starej populacji do nowej w zamian za najgorszego
	// O ile jest taka potrzeba
	if(genitor)
	{
		if(popDst[best(popDst)]->objective > popSrc[best(popSrc)]->objective)
		{
			*popDst[worst(popDst)] = *popSrc[best(popSrc)];
		}
	}

	// Aktualizuj dane dla populacji
	sort(popDst);
	calcStats(popDst);
	fitness(popDst);
}
// -------------------------------------------------------------------------
void Problem::nextGen()
{
	// "podwojne buforowanie" - unikniecie kopiowania
	// dla indexPop==0 populacja ktora zaraz zostanie wygenerowana
	// bedzie tmpPop, dla indexPop==1 bedzie to pop
	Population& popSrc = (indexPop ? tmpPop : pop);
	Population& popDst = (indexPop ? pop : tmpPop);

	if(ssMethod == SS_ROULETTE)
		preselectRoulette(popSrc);
	else if(ssMethod == SS_SUS)
		preselectSus(popSrc);

	(this->*(pfnNextGen))(popSrc, popDst);
}
// -------------------------------------------------------------------------
uint32 Problem::objectiveScore(Genome& gen)
{
	uint32 max = 0;
	for(uint32 k = 0; k < numJobs; ++k)
	{
		int lastOpIdx = gen.rows[k].genes.size()-1;
		int lastOpsJobIdx = jobs[k].ops.size()-1;
		int machineIdx = gen.rows[k].genes[lastOpIdx].machine;

		uint32 lastOpStart = gen.rows[k].genes[lastOpIdx].time;
		uint32 lastOpEnd = lastOpStart + jobs[k].ops[lastOpsJobIdx].costs[machineIdx];

		max = std::max(max, lastOpEnd);
	}
	return max;
}
// -------------------------------------------------------------------------
uint32 Problem::computeStartingTime(Genome& gen)
{
	// Contains the deadline od the last operation scheduled on machine M[k]
	uint32* DMk = new uint32[numMachines];
	// Containst the deadline of the last operation scheduled on Job[j]
	uint32* Tf = new uint32[numJobs];

	for(uint32 i = 0; i < numMachines; ++i)
		DMk[i] = 0;
	for(uint32 i = 0; i < numJobs; ++i)
		Tf[i] = 0;

	for(uint32 i = 0; i < maxOps; ++i)
	{
		for(uint32 j = 0; j < numJobs; ++j)
		{
			if(jobs[j].numOps <= i)
				continue;

			int machineIdx = gen.rows[j].genes[i].machine;

			// calculate
			{
				if(Tf[j] < DMk[machineIdx])
					gen.rows[j].genes[i].time = DMk[machineIdx];
				else
					gen.rows[j].genes[i].time = Tf[j];
			}
			// update
			{
				Tf[j] = gen.rows[j].genes[i].time + jobs[j].ops[i].costs[machineIdx];
				DMk[machineIdx] = gen.rows[j].genes[i].time + jobs[j].ops[i].costs[machineIdx];
			}
		}
	}

	// makespan
	uint32 makespan = Tf[getMaxIdx(Tf, numJobs)];

	delete [] DMk;
	delete [] Tf;

	return (gen.objective = makespan);
}
// -------------------------------------------------------------------------
void Problem::mutate(const Genome& in, Genome& out)
{
	out = in;

	// Calculate load of the machine before mutation
	uint32* machineLoad = new uint32[numMachines];
	for(uint32 i = 0; i < numMachines; ++i)
		machineLoad[i] = 0;

	for(uint32 i = 0; i < numJobs; ++i)
	{
		for(uint32 j = 0; j < jobs[i].ops.size(); ++j)
		{
			const Gene& g = in.rows[i].genes[j];
			int cost = jobs[i].ops[j].costs[g.machine];
			machineLoad[g.machine] += cost;
		}
	}

	uint32 maxMachineLoad = getMaxIdx(machineLoad, numMachines);
	uint32 minMachineLoad = getMinIdx(machineLoad, numMachines);

	// * Step 1
	// Choose randomly one genome and one operation
	// from the set of operations assigned to a machine with a high load.

	std::vector<Gene*> geneSet;
	for(uint32 i = 0; i < numJobs; ++i)
	{
		for(uint32 j = 0; j < jobs[i].ops.size(); ++j)
		{
			if(in.rows[i].genes[j].machine == maxMachineLoad)
				geneSet.push_back(&out.rows[i].genes[j]);
		}
	}

	uint32 random;
	if(geneSet.size() > 1)
		random = rnd.random(0, geneSet.size() - 1);
	else
		random = 0;

	// * Step 2
	// Assign this operation to another machine with a small load, if possible
	Gene* g = geneSet[random];
	g->machine = minMachineLoad;

	computeStartingTime(out);

	delete [] machineLoad;
}
// -------------------------------------------------------------------------
void Problem::columnCrossover(const Genome& mom, const Genome& dad, 
	Genome& kid1, Genome& kid2)
{
	// * Step 1
	// Choose randomly one operation.
	rnd.setBound(0, maxOps - 1);
	uint32 op = rnd.random();
	
	// * Step 2
	// Operation 'op' of all the jobs in C1 (resp. C2) received 
	// the same machines assigned to Operation 'op' of all the jobs
	// of P1 (resp. P2).
	for(uint32 i = 0; i < numJobs; ++i)
	{
		if(jobs[i].ops.size() <= op)
			continue;
		kid1.rows[i].genes[op] = mom.rows[i].genes[op];
		kid2.rows[i].genes[op] = dad.rows[i].genes[op];
	}

	// * Step 3
	// Copy the remainder of the machines assigned to other operations of P2 (resp. P1
	// in the same operations of C1 (resp. C2)

	for(uint32 i = 0; i < numJobs; ++i)
	{
		for(uint32 j = 0; j < jobs[i].ops.size(); ++j)
		{
			if(j == op)
				continue;
			kid1.rows[i].genes[j] = dad.rows[i].genes[j];
			kid2.rows[i].genes[j] = mom.rows[i].genes[j];
		}
	}

	computeStartingTime(kid1);
	computeStartingTime(kid2);
}
// -------------------------------------------------------------------------
void Problem::rowCrossover(const Genome& mom, const Genome& dad, 
	Genome& kid1, Genome& kid2)
{
	// * Step 1
	// Choose randomly job
	rnd.setBound(0, numJobs - 1);
	uint32 job = rnd.random();

	// * Step 2
	// The operation of 'job' in C1 (resp. C2) received the same
	// machines as those assigned to 'job' of P1 (resp. P2)

	for(uint32 j = 0; j < jobs[job].ops.size(); ++j)
	{
		kid1.rows[job].genes[j] = mom.rows[job].genes[j];
		kid2.rows[job].genes[j] = dad.rows[job].genes[j];
	}

	// * Step 3
	// Copy the remainder of the machinees assigned to the operation
	// of the other jobs of P1 (resp. P2) in the same jobs of C2 (resp. C1)

	for(uint32 i = 0; i < numJobs; ++i)
	{
		if(i == job)
			continue;

		for(uint32 j = 0; j < jobs[i].ops.size(); ++j)
		{

			kid1.rows[i].genes[j] = dad.rows[i].genes[j];
			kid2.rows[i].genes[j] = mom.rows[i].genes[j];
		}
	}

	computeStartingTime(kid1);
	computeStartingTime(kid2);
}
// -------------------------------------------------------------------------
bool Problem::outputToMatlab(const char* filename, uint32 genomeIndex)
{
	assert(genomeIndex < pop.size());

	// TODO: na razie pierwszy z brzegu
	Population& popSrc = (indexPop ? tmpPop : pop);
	Genome& c = *popSrc[genomeIndex];

	return outputToMatlab(filename, c);
}
// -------------------------------------------------------------------------
bool Problem::outputToMatlab(const char *filename, Genome &gen)
{
	FILE* fp = fopen(filename, "w+");
	if(!fp)
		return false;

	for(uint32 i = 0; i < numJobs; ++i)
	{
		float r = colors[i].r;
		float g = colors[i].g;
		float b = colors[i].b;

		fprintf(fp, "color = [%f %f %f];\n", r, g, b);
		//printf("color = [%f %f %f];\n", r, g, b);

		for(uint32 j = 0; j < jobs[i].ops.size(); ++j)
		{
			uint32 machine = gen.rows[i].genes[j].machine;
			uint32 startTime = gen.rows[i].genes[j].time;
			uint32 procTime = jobs[i].ops[j].costs[machine];

			fprintf(fp, "T%d%d = struct('StartTime', %d, 'ProcTime', %d,"
				"'Machine', %d, 'Name', '%d/%d', 'Color', color);\n",
				i, j, startTime, procTime, machine, i+1, j+1);
		}
		fprintf(fp, "\n");
	}

	fprintf(fp, "\nT=[");
	for(uint32 i = 0; i < numJobs; ++i)
	{
		for(uint32 j = 0; j < jobs[i].ops.size(); ++j)
		{
			fprintf(fp, " T%d%d", i, j);
		}
	}

	fprintf(fp, " ];\nplotgantt(T, %d);\n", numMachines);
	fclose(fp);
	return true;
}
// -------------------------------------------------------------------------

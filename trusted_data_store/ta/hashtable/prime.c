#include <prime.h>

size_t next_prime(const size_t n)
{
	for(size_t i = 0; i < sizeof(primes); i++)
	{
		if(primes[i] >= n) return primes[i];
	}

	return 0;
}

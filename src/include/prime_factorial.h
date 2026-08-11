#pragma once
#include <iostream>
#include <vector>
using namespace std;

bool is_Prime(int _x)
{
	if (_x <= 1) // invalid input
		return false;

	for (int i = 2; i <= _x / 2; i++) // process all potential divisors
	{
		if (_x % i == 0)
			return false;
	}

	return true; // no divisor found, therfore it's a prime number
}

int prime_factorial(int _number)
{
	vector<int> Factors; // vector to store all the prime factors of a number
	// iterate from 2 to half of the number as there can be no factor
	// greater than half of the number.
	for (int i = 2; i <= _number / 2; i++)
	{
		if (_number % i == 0) // check if number is factor
		{
			if (is_Prime(i) == true) // check if the factor is also a prime number
			{
				Factors.push_back(i); // add the value in the vector
			}
		}
	}
	int max = 1;

	for (int i = 0; i < Factors.size(); i++) // iterate the vector to find largest prime factor
	{
		if (Factors[i] > max)
		{
			max = Factors[i];
		}
	}

	// output the largest prime factor
	cout << "Largest prime factor = " << max << endl;
	return max;
}

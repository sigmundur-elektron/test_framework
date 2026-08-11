#pragma once
int factorial(int _number)
{
	return _number <= 1 ? _number : factorial(_number - 1) * _number;
}

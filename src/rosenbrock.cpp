#include "rosenbrock.h"

double rosenbrock(Eigen::VectorXd x)
{
	if (x.size() < 2)
	{
		std::cout << "dimension must be greater one" << std::endl;
		return 0;
	}
	else
	{
		double f = 0;
		for (int i = 0; i < x.size() - 1; i++)
			f += 100 * (pow((pow((x(i, 0)), 2) - x(i + 1, 0)), 2)) + pow((x(i, 0) - 1), 2);
		return f;
	}
}

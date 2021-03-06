#include "cmaes.h"

using namespace Eigen;

Cmaes::Ptr cmaesOptimizer;

Cmaes::Cmaes(void) : Optimizer("CMAES", CMAES) 
{
}

const std::string& Cmaes::getName() const 
{
	return name;
}

MatrixXd Cmaes::sort(MatrixXd& tablica, MatrixXd& index)
{
	MatrixXd j_i(1, tablica.size());
	j_i = tablica;
	std::sort(tablica.data(), tablica.data() + tablica.size());

	for (int i = 0; i < tablica.size(); i++)
		for (int j = 0; j < tablica.size(); j++)
			if (j_i(0, j) == tablica(0, i))
				index(0, i) = j;
	return tablica;
}

std::pair<MatrixXd, VectorXd> Cmaes::eig(const MatrixXd& A, const MatrixXd& B)
{
	Eigen::GeneralizedSelfAdjointEigenSolver<MatrixXd> solver(A, B);
	MatrixXd V = solver.eigenvectors();
	VectorXd D = solver.eigenvalues();

	return std::make_pair(V, D);
}

void Cmaes::setParameters()
{	
	//Strategy parameter setting: Selection
	stopeval = 1000 * pow(N, 2); //stop after stopeval number of function evaluations
	std::srand(time(NULL)); 
	xmean = VectorXd::Random(N); 
	xmean = xmean.cwiseAbs(); //objective variables initial point
	lambda = 4 + floor(3 * log(N)); //population size, offspring number
	mu = lambda / 2; //number of parents/points for recombination
	
	weights.resize(mu); //muXone array for weighted recombination
	for (int i = 0; i < mu; i++)
		weights(i) = log(mu + 0.5) - log(i + 1);
		
	double sum_weights = weights.sum(); //normalize recombination weights array
	weights /= sum_weights; 

	double sum_weights_pow = 0; //variance-effectiveness of sum w_i x_i
	for (int i = 0; i < mu; i++)
		sum_weights_pow += pow(weights(i), 2);
	mueff = 1 / sum_weights_pow; 


	//Strategy parameter setting: Adaptation
	cc = (4 + (mueff / N)) / (N + 4 + (2 * (mueff / N))); //time constant for cumulation for C
	cs = (mueff + 2) / (N + mueff + 5); //t-const for cumulation for sigma control
	c1 = 2 / (pow((N + 1.3), 2) + mueff); //learning rate for rank-one update of C
	cmu = std::min(1 - c1, 2 * (mueff - 2 + (1 / mueff)) / (pow((N + 2), 2) + mueff)); //and for rank-mu update
	damps = 1 + 2 * std::max((double)0, sqrt((mueff - 1) / (N + 1)) - 1) + cs; //damping for sigma 


	//Initialize dynamic (internal) strategy parameters and constants
	pc = VectorXd::Zero(N); //evolution paths for C and sigma
	ps = VectorXd::Zero(N);
	B = MatrixXd::Identity(N, N); //B defines the coordinate system
	D = VectorXd::Ones(N); //diagonal D defines the scaling
	C = MatrixXd::Identity(N, N); //covariance matrix C
	invsqrtC = MatrixXd::Identity(N, N); //C^-1/2
	arindex.resize(1, lambda);
	arx.resize(N, lambda);
	arfitness.resize(1, lambda);
	arx_N_x_mu.resize(N, mu);
	eigeneval = 0; //track update of B and D
	chiN = pow(N, 0.5)*(1 - (1 / (4.0 * N)) + (1 / (21 * pow(N, 2)))); //expectation of ||N(0,I)|| == norm(randn(N,1))
	
}

void Cmaes::genOffspring(std::map<std::string, std::function<double(Eigen::VectorXd)>>  funcMap, std::string function)
{
	//Generate and evaluate lambda offspring
	for (int k = 0; k < lambda; k++)
	{
		VectorXd rand_ = VectorXd::Random(N);
		//SPRAWDZIC --> dochodzi do spełnienia warunku max(D)>10000000*min(D) przerywającego główną funkcję genLoop()
		//	rand_ = D.cwiseProduct(rand_); // matlab: rand_ = D.*rand_
		VectorXd arx_ = xmean + sigma * B * (rand_); //m + sig * Normal(0,C)
		for (int i = 0; i < N; ++i)
			arx(i, k) = arx_(i); 
		arfitness(0, k) = funcMap[function](arx_); //objective function call
		counteval++;
	}
}

void Cmaes::updateXmean()
{
	//recombination, new mean value
	xold = xmean; //recombination, new mean value
	for (int i = 0; i < mu; ++i)
		for (int j = 0; j < N; ++j)
		{
			int a = arindex(0, i);
			arx_N_x_mu(j, i) = arx(j, a);
		}
	xmean = arx_N_x_mu*weights; 
}

void Cmaes::updateEvoPath()
{
	//Cumulation: Update evolution paths
	ps = (1 - cs)*ps + sqrt(cs*(2 - cs)*mueff)*invsqrtC*(xmean - xold) / sigma;
	ps_norm = ps.norm();
	double c__ = 1 - pow((1 - cs), (2 * counteval / lambda));
	if (((ps_norm / sqrt(c__)) / chiN) < (1.4 + (2.0 / (N + 1))))
		hsig = 1;
	else
		hsig = 0;
	pc = (1 - cc)*pc + hsig*sqrt(cc*(2 - cc)*mueff)*(xmean - xold) / sigma;
}

void Cmaes::updateMatrixC()
{
	//Adapt covariance matrix C
	MatrixXd repmat(N, mu);
	for (int i = 0; i < mu; ++i)
		for (int j = 0; j < N; ++j)
			repmat(j, i) = xold(j, 0);

	MatrixXd artmp(N, mu);
	artmp = (1 / sigma) * (arx_N_x_mu - repmat);

	C = (1 - c1 - cmu)*C //regard old matrix 
		+ c1 * (pc*pc.transpose() //plus rank one update
		+ (1 - hsig)*cc*(2 - cc)*C) //minor correction if hsig==0
		+ cmu *artmp* weights.asDiagonal()*artmp.transpose(); //plus rank mu update
}

void Cmaes::updateOtherMatrices()
{
	//Decomposition of C into B*diag(D.^2)*B' (diagonalization)
	eigeneval = counteval;
	MatrixXd CC(N, N);
	CC = C.triangularView<Eigen::Upper>();
	MatrixXd CCC(N, N);
	CCC = C.triangularView<Eigen::Upper>();

	for (int i = 0; i < N; ++i)
		CCC(i, i) = 0;
	C = CC + CCC.transpose(); //enforce symmetry

	//DO SPRAWDZENIA!!!
	std::pair<MatrixXd, VectorXd> BD;
	MatrixXd BB = MatrixXd::Identity(N, N);
	BD = eig(C, BB); //eigen decomposition, B==normalized eigenvectors
	B = BD.first;
	D = BD.second;

	for (int i = 0; i < D.size(); ++i)
		D(i) = sqrt(D(i)); //D is a vector of standard deviations now

	MatrixXd D_diag = MatrixXd::Zero(N, N);
	for (int i = 0; i < N; ++i)
		D_diag(i, i) = pow(D(i), -1);
	invsqrtC = B * D_diag * B.transpose();
}

void Cmaes::genLoop(std::map<std::string, std::function<double(Eigen::VectorXd)>>  funcMap, std::string function)
{
	setParameters();

	//Generation Loop
	counteval = 0;
	while (counteval < stopeval)
	{
		//Generate and evaluate lambda offspring
		genOffspring(funcMap, function); 
		
		//Sort by fitness and compute weighted mean into xmean
		sort(arfitness, arindex); //minimization

		//recombination, new mean value
		updateXmean();

		//Cumulation: Update evolution paths
		updateEvoPath();

		//Adapt covariance matrix C
		updateMatrixC();

		//Adapt step size sigma
		sigma *= exp((cs / damps)*((ps_norm / chiN) - 1));

		//Decomposition of C into B*diag(D.^2)*B' (diagonalization)
		if ((counteval - eigeneval) > (lambda / (c1 + cmu) / N / 10)) // to achieve O(N^2)
			updateOtherMatrices();

		//Break, if fitness is good enough or condition exceeds 1e14, better termination methods are advisable 
		if (arfitness(0) <= stopfitness || D.maxCoeff()>(10000000 * D.minCoeff()))
			break;
	}
}

void Cmaes::output()
{
	//Return best point of last iteration.
	VectorXd xmin(N);
	for (int i = 0; i < N; ++i)
		xmin(i) = arx(i, arindex(0));
	std::cout << "min: " << std::endl << xmin;
}

Optimizer* createCMAES(void) 
{
    cmaesOptimizer.reset(new Cmaes());
    return cmaesOptimizer.get();
}

Optimizer* createCMAES(std::string configFile) 
{
    cmaesOptimizer.reset(new Cmaes(configFile));
    return cmaesOptimizer.get();
}

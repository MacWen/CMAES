#ifndef cmaes_h
#define cmaes_h
#include <iostream>
#include <cmath>
#include <cstdio>
#include <stdlib.h>     
#include <algorithm>
#include "time.h"
#include <string>
#include <../3rdParty/Eigen/Dense>
#include "../3rdParty/tinyXML/tinyxml2.h"
#include "optimizer.h"
#include <memory>

using namespace Eigen;

Optimizer* createCMAES(void);
Optimizer* createCMAES(std::string configFile);

class Cmaes : public Optimizer
{
	private:
		int N;
		double sigma;
		double stopfitness;
		double stopeval;
		int counteval;
		int eigeneval;
		double chiN;
		int lambda;
		int mu;
		double mueff;
		double cc, cs, c1, cmu, cmu_a, cmu_b, damps;
		double ps_norm, hsig;
		
		VectorXd xmean;
		VectorXd weights;		
		MatrixXd arindex;
		MatrixXd arx;
		MatrixXd arfitness;
		MatrixXd arx_N_x_mu;
		VectorXd xold;
		VectorXd pc; // evolution paths for C and sigma
		VectorXd ps;
		MatrixXd B; // B defines the coordinate system
		VectorXd D; // diagonal D defines the scaling
		MatrixXd C; // covariance matrix C
		MatrixXd invsqrtC;

		MatrixXd sort(MatrixXd& tablica, MatrixXd& index);
		std::pair<MatrixXd, VectorXd> eig(const MatrixXd& A, const MatrixXd& B);
		void genOffspring(std::map<std::string, std::function<double(Eigen::VectorXd)>>  funcMap, std::string function);
		void updateXmean();
		void updateEvoPath();
		void updateMatrixC();
		void updateOtherMatrices();
		
	public:
		/// Pointer
		typedef std::unique_ptr<Cmaes> Ptr;

       	/// Construction
       	Cmaes(void);

       	/// Construction
       	Cmaes(std::string configFilename) : Optimizer("CMAES", CMAES) 
		{
			tinyxml2::XMLDocument doc;
			std::string filename = "../resources/" + configFilename;
			doc.LoadFile( filename.c_str() ); 
  			if (doc.ErrorID())
       			std::cout << "unable to load optimizer config file.\n";
			tinyxml2::XMLElement * model = doc.FirstChildElement( "CMAES" );
			model->FirstChildElement( "parameters" )->QueryIntAttribute("N", &N); //number of objective variables/problem dimension
			model->FirstChildElement( "parameters" )->QueryDoubleAttribute("sigma", &sigma); //coordinate wise standard deviation (step size)
			model->FirstChildElement( "parameters" )->QueryDoubleAttribute("stopfitness", &stopfitness); //stop if fitness < stopfitness (minimization)
       	}

		virtual const std::string& getName() const;
      	void setParameters();
		void genLoop(std::map<std::string, std::function<double(Eigen::VectorXd)>>  funcMap, std::string function); 
		void output();
};

#endif

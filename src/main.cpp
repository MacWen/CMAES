#include <iostream>
#include "time.h"
#include "rosenbrock.h"
#include "cmaes.h"

int main()
{
 	try 
	{
		tinyxml2::XMLDocument config;
		config.LoadFile("../resources/configGlobal.xml");
		if (config.ErrorID())
			std::cout << "unable to load config file.\n";
        std::string optimizerType(config.FirstChildElement( "Optimizer" )->FirstChildElement( "name" )->GetText());
		std::string function(config.FirstChildElement( "Function" )->FirstChildElement( "name" )->GetText());

        Optimizer* optimizer;
        if (optimizerType == "CMAES") 
		{
        	std::string configFile(config.FirstChildElement( "Optimizer" )->FirstChildElement( "calibrationFile" )->GetText());
        	optimizer = createCMAES(configFile);
        }
       	else // Default
        	optimizer = createCMAES();
        	
		std::map<std::string, std::function<double(Eigen::VectorXd)>>  funcMap =
		{
			{ "rosenbrock", rosenbrock}
		};
		
        // create objects and print configuration
        std::cout << "Current optimizer: " << optimizer->getName() << std::endl;
		std::cout << "Current function: " << function << std::endl;
        
		clock_t start, stop;
		double czas;
		start = clock();
		optimizer->genLoop(funcMap, function);
		stop = clock();
		czas = (double)(stop - start) / CLOCKS_PER_SEC;
		optimizer->output();
		std::cout << std::endl << "Execution time: " << czas << " s" << std::endl;

    	}

	catch (const std::exception& ex) 
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

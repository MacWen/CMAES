#ifndef optimizer_h
#define optimizer_h
#include <functional>
#include <map>
#include <../3rdParty/Eigen/Dense>

class Optimizer
{
	public:

		// Optimizer type
		enum Type 
		{
			CMAES
		};

    	// overloaded constructor
      	Optimizer(const std::string _name, Type _type) : name(_name), type(_type) {};

      	// Name of the optimizer
      	virtual const std::string& getName() const = 0;

      	virtual void setParameters() = 0;

		virtual void genLoop(std::map<std::string, std::function<double(Eigen::VectorXd)>>  funcMap, std::string function) = 0; 

		virtual void output() = 0;

		// Virtual descrutor
		virtual ~Optimizer() {}

	protected:
		// Optimizer type
      	Type type;

       	// Optimizer name
       	const std::string name;
};

#endif

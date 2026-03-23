#pragma once
#include <vector>
#include <string>
#include "molecule.hpp"

enum class SolutionComponentState {
	Gas,
	InSolution, // either as solvent or solute
	Solid
};

struct SolutionComponent {
	unsigned moleculeId; 
	float numMols;
	float surfaceAreaPerMol;
};

// could represent atmosphere in a room/pressurized container, or the contents of a beaker/test tube/etc.
class ChemicalSolution {
public:
	float volume; // in litres
	float temperature; // in kelvin
	std::vector<SolutionComponent> molecules;

	// for something like a beaker, this would only contain the ChemicalSolution for the room the beaker is in.
	// for a room, this would contain the ChemicalSolutions of all the solutions/etc. in the room. 
	std::vector<ChemicalSolution*> neighbors;

	// does one RK4 timestep
	ChemicalSolution StepSolution(float timestep);
};

// represents the system at a specific point in time.
class ChemicalSystem {
public:
	std::vector<ChemicalSolution> solutions;
	ChemicalSystem StepSystem(float timestep);
};
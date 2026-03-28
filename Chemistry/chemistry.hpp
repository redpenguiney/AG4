#pragma once
#include <vector>
#include <string>
#include <memory>
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

	// for a room, this would contain the ChemicalSolutions of all the solutions/etc. in the room. 
	std::vector<ChemicalSolution*> children;

	// for the contents of a beaker, this would be the atmosphere of the room the beaker is in.
	ChemicalSolution* parent;

	// does one RK4 timestep
	std::unique_ptr<ChemicalSolution> StepSolution(float timestep);
};

// represents the system at a specific point in time.
class ChemicalSystem {
public:
	std::vector<std::unique_ptr<ChemicalSolution>> solutions;
	ChemicalSystem StepSystem(float timestep);
};
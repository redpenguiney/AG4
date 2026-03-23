#include "chemistry.hpp"
#include "reaction.hpp"

int main() {
	InitializeMoleculeTypes();
	InitializeReactions();

	ChemicalSystem system;

	ChemicalSolution atmosphere;
	atmosphere.volume = 1000.0f;
	atmosphere.temperature = 273.0f;
	atmosphere.molecules.push_back(SolutionComponent{
		.moleculeId = moleculeIdsByName["dioxygen"],
		.numMols = 0.0155f * atmosphere.volume
		});

	system = system.StepSystem(1.0f);

	return EXIT_SUCCESS;
}
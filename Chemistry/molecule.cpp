#include "molecule.hpp"

void InitializeMoleculeTypes() {

	moleculeTypes[1] = MoleculeInfo{
		.name = "dioxygen",
		.id = 1,
		.molecularWeight = 15.999f * 2.0f
	};
	moleculeIdsByName["dioxygen"] = 1;

	moleculeTypes[2] = MoleculeInfo{
		.name = "ozone",
		.id = 2,
		.molecularWeight = 15.999f * 3.0f
	};
	moleculeIdsByName["ozone"] = 2;

	moleculeTypes[3] = MoleculeInfo{
		.name = "atomic oxygen",
		.id = 3,
		.molecularWeight = 15.999f
	};
	moleculeIdsByName["atomic oxygen"] = 3;

}

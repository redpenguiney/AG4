#pragma once
#include <unordered_map>
#include <string>

enum class AtomType {
	Null = 0,
	H = 1,
	He = 2,
	Li = 3,
	Be = 4,
	B = 5,
	C = 6,
	N = 7,
	O = 8
};

struct MoleculeInfo {
	std::string name = "DEFAULT";
	unsigned id = 0;
	float molecularWeight = 1;
	//float molarDensity = 1; // L/mol
};

inline std::unordered_map<std::string, unsigned> moleculeIdsByName;
inline std::unordered_map<unsigned, MoleculeInfo> moleculeTypes;

void InitializeMoleculeTypes();
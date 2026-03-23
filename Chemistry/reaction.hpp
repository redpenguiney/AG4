#pragma once
#include <tuple>
#include <unordered_map>

// (in joules per kelvin per mol)
constexpr float UNIVERSAL_GAS_CONSTANT = 8.31446261815324f;

struct UnimolecularReaction {
	unsigned reactantId;
	std::vector<std::pair<unsigned, unsigned>> products; // first is id, second is count
	float activationEnergy; // in J/mol
	float arrheniusFactor; // in seconds^-1, temperature-independent reaction rate multiplier
};

struct BimolecularReaction {
	unsigned reactant1Id;
	unsigned reactant2Id;
	unsigned productId;
	float activationEnergy; // in J/mol
	float arrheniusFactor; // in seconds^-1, temperature-independent reaction rate multiplier
};

// key is reactantId
inline std::unordered_map<unsigned, std::vector<UnimolecularReaction>> unimolecularReactions;

unsigned long long GetBimolecularReactionKey(unsigned reactant1Id, unsigned reactant2Id) {
	return reactant1Id & (static_cast<unsigned long long>(reactant2Id) << 32);
}

// key is GetBimolecularReactionKey(reactant1Id, reactant2Id)
// for a given bimolecular reaction, it will only have ONE of the two possible orders (otherwise reaction would happen twice as fast)
inline std::unordered_map<unsigned long long, std::vector<BimolecularReaction>> bimolecularReactions;

void InitializeReactions();
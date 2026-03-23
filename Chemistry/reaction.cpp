#include "reaction.hpp"
#include "molecule.hpp"

void InitializeReactions() {
	unimolecularReactions[moleculeIdsByName["dioxygen"]].push_back(UnimolecularReaction{
		.reactantId = moleculeIdsByName["dioxygen"],
		.products = std::vector {std::make_pair(moleculeIdsByName["atomic oxygen"], 2u),},
		.activationEnergy = 498000,
		.arrheniusFactor = 1 // TODO
		});

	bimolecularReactions[GetBimolecularReactionKey(moleculeIdsByName["dioxygen"], moleculeIdsByName[""])]
}

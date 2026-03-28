#include "chemistry.hpp"
#include "reaction.hpp"
#include <vector>

std::unique_ptr<ChemicalSolution> ChemicalSolution::StepSolution(float timestep) {
    std::unique_ptr<ChemicalSolution> newSolution = std::make_unique<ChemicalSolution>();
    newSolution->volume = volume;

    // collect all molecules that could react with a molecule in this solution
    std::vector<std::pair<SolutionComponent*, ChemicalSolution*>> possibleReactants;

    std::vector<UnimolecularReaction*> uniReactions;

    // get all unimolecular reactions
    for (auto& [mol, sol] : possibleReactants) {
        if (unimolecularReactions.contains(mol->moleculeId)) {
            for (auto& rxn : unimolecularReactions[mol->moleculeId]) {
                uniReactions.push_back(&rxn);
            }
        }
    }

    std::vector<BimolecularReaction*> internalBireactions;
    for (unsigned i = 0; i < possibleReactants.size(); i++)) {
        auto& [mol1, sol1] : possibleReactants[i];
        for (unsigned j = i+1; j < possibleReactants.size(); j++)) {
            auto& [mol2, sol2] : possibleReactants[j];
            auto idx1 = GetBimolecularReactionKey(mol1->moleculeId, mol2->moleculeId);
            auto idx2 = GetBimolecularReactionKey(mol2->moleculeId, mol1->moleculeId);

            if (biimolecularReactions.contains(idx1)) {
                for (auto& rxn : bimolecularReactions[idx1]) {
                    internalBireactions.push_back(&rxn);
                }
            }
            else if (bimolecularReactions.contains(idx2) {
                for (auto& rxn : bimolecularReactions[idx2]) {
                    internalBireactions.push_back(&rxn);
                }
            }
        }
    }

    std::vector<BimolecularReaction*> bireactionsFeedingParent;

    return newSolution;
}

ChemicalSystem ChemicalSystem::StepSystem(float timestep)
{
    ChemicalSystem newSystem{};
    std::unordered_map<SolutionComponent*, SolutionComponent*> oldToNew;
    for (auto& s : solutions) {
        newSystem.solutions.push_back(s->StepSolution(timestep));
    }


    return newSystem;
}

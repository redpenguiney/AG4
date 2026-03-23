#include "chemistry.hpp"
#include <vector>

ChemicalSolution ChemicalSolution::StepSolution(float timestep) {
    ChemicalSolution newSolution {};
    newSolution.volume = volume;

    // collect all molecules that could react with a molecule in this solution
    std::vector<std::pair<SolutionComponent*, ChemicalSolution*>> possibleReactants;

    // get all unimolecular reactions
    for (auto& [mol, sol] : possibleReactants) {

    }



    return newSolution;
}

ChemicalSystem ChemicalSystem::StepSystem(float timestep)
{
    ChemicalSystem newSystem{};
    for (auto& s : solutions) {
        newSystem.solutions.push_back(s.StepSolution(timestep));
    }
    return newSystem;
}

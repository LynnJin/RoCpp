//
//  SolverModeller.cpp
//  RobustOptimizationPlatform
//
// This software is Copyright © 2020 Phebe Vayanos. All Rights Reserved.
// Software created by Phebe Vayanos, Qing Jin, and George Elissaios
//

#include "gurobi_c++.h"
#include "IncludeFiles.hpp"
#include "SolverModeller.hpp"
#include "GurobiModeller.hpp"
#include <boost/thread.hpp>

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%% SOLVER PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%% Getter Functions %%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void SolverParams::printMainSolverParamsToScreen() const
{
    cout << "=========================================================================== " << endl;
    cout << "=========================================================================== " << endl;
    cout << "=========================== SOLVER PARAMS ================================= " << endl;
    cout << "=========================================================================== " << endl;
    cout << "=========================================================================== " << endl;
    
    
    cout << "Lazy NACs: "  << m_useLazyNACs << endl;
    if (m_timeLimit.first)
        cout << "Time Limit:" << m_timeLimit.second << endl;
    if (m_epGapLimit.first)
        cout << "AGap Limit:" << m_epGapLimit.second << endl;
    if (m_epOptLimit.first)
        cout << "Opt Limit:" << m_epOptLimit.second << endl;
    if (m_epRHSLimit.first)
        cout << "RHS Limit:" << m_epRHSLimit.second << endl;
    if (m_epIntLimit.first)
        cout << "Int Limit:" << m_epIntLimit.second << endl;
    cout << "SOS eps:" << m_SOSeps << endl;
    
    cout << "=========================================================================== " << endl;
    
}

string SolverParams::getColumnTitlesOfOutputDataFile() const
{
    string out("LazyNACs, ");
    out += "TimeLimit";
    return out;
}

string SolverParams::getParams(string delimitter) const
{
    string out(boost::lexical_cast<string>(m_useLazyNACs));
    out += delimitter;
    out += boost::lexical_cast<string>(m_timeLimit.second);
    return out;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%% SOLVER MODELLER INTERFACE %%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%% Getter Functions %%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double SolverModellerIF::getSolution(string dvName) const
{
    if(m_solution.find(dvName) == m_solution.end())
        throw MyException("Decision variable does not exist");
    
    return m_solution.find(dvName)->second;
}

int SolverModellerIF::getThreadsToRun() const
{
    int numThreads(static_cast<int>( boost::thread::hardware_concurrency() ));

    if (numThreads>2)
        numThreads -= 1;
    else if (numThreads<1)
        numThreads = 1;
    
    
    return numThreads;
}

uint SolverModellerIF::getOptStatus() const
{
    if(!m_problemSolved)
        throw MyException("No problem is solved now");
    
    return m_results.m_optStatus;
}

double SolverModellerIF::getOptValue() const
{
    if(getOptStatus() == 2)
        return m_results.m_optValue;
    else {
        cout << "No optimal value in status: " + to_string(getOptStatus() ) << endl;
        return 0.0;
    }
}

double SolverModellerIF::getMIPGap() const
{
    if(!m_problemSolved)
        throw MyException("No problem is solved now");
    
    if(m_results.m_isMIP)
        return m_results.m_MIPGap;
    else{
        cout << "No integer variable in this problem." << endl;
        return 0.0;
    }
}

double SolverModellerIF::getSolvingTime() const
{
    if(!m_problemSolved)
        throw MyException("No problem is solved now");
    
    return m_results.m_solveTime;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%% Setter Functions %%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void SolverModellerIF::setParameters(GRBModel &Model) const
{
    
    Model.getEnv().set(GRB_IntParam_Threads, getThreadsToRun());
    
    Model.getEnv().set(GRB_IntParam_OutputFlag,boost::lexical_cast<int>(m_pSParams.getVerbose()));
    
    if (getEpAGapLimit().first)
        Model.getEnv().set(GRB_DoubleParam_MIPGapAbs,getEpAGapLimit().second);
    
    if (getEpOptLimit().first)
        Model.getEnv().set(GRB_DoubleParam_OptimalityTol,getEpOptLimit().second);
    
    if (getEpRHSLimit().first)
        Model.getEnv().set(GRB_DoubleParam_FeasibilityTol,getEpRHSLimit().second);
    
    if (getEpIntLimit().first)
        Model.getEnv().set(GRB_DoubleParam_IntFeasTol,max(getEpIntLimit().second,1.e-9));
    
    if (getTimeLimit().first)
        Model.getEnv().set(GRB_DoubleParam_TimeLimit,getTimeLimit().second);
    
    if (getEpGapLimit().first)
        Model.getEnv().set(GRB_DoubleParam_MIPGap,getEpGapLimit().second);
}

void SolverModellerIF::saveResults(GRBModel& model, pair<bool,string> writeSlnToFile) const
{
    if (model.get(GRB_IntAttr_Status) == GRB_OPTIMAL)
    {
        if (writeSlnToFile.first)
        {
            model.write((writeSlnToFile.second+"All.sol").c_str());
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%% CREATE SOLVER FUNCTION %%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

boost::shared_ptr<SolverModellerIF> InstantiateSolverModeller( string solver, const SolverParams &pSParams, bool useLazyNACs )
{
    if (solver=="cplex")
        throw MyException("CPLEX not available");

    else if (solver=="gurobi")
        return  boost::shared_ptr<SolverModellerIF>( new  GurobiModeller(pSParams, useLazyNACs ) );
    
    throw MyException("unknown solver");
}

boost::shared_ptr<SolverModellerIF> InstantiateSolverModeller( string solver, bool useLazyNACs )
{
    if (solver=="cplex")
        MyException("CPLEX not available");

    else if (solver=="gurobi")
        return  boost::shared_ptr<SolverModellerIF>( new  GurobiModeller( useLazyNACs ) );
    
    throw MyException("unknown solver");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%% PRIORITIES FUNCTION %%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


void setPriorities(map<string,GRBVar>& VarMap, const map<string,int>& priorities)
{
    for (map<string,int>::const_iterator pit = priorities.begin(); pit != priorities.end(); pit++)
    {
        map<string,GRBVar>::iterator vit = VarMap.find(pit->first);
        
        if (vit == VarMap.end() )
            throw MyException("GRBVar not found in map");
        
        vit->second.set(GRB_IntAttr_BranchPriority,pit->second);
    }
}
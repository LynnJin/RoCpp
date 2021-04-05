//
//  main.cpp
//  ROCPP
//
// This software is Copyright © 2020 Phebe Vayanos. All Rights Reserved.
// Software created by Phebe Vayanos, Qing Jin, and George Elissaios
//

#include <stdio.h>
#include <fstream>
#include "ROCPP.h"
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>



int main()
{ return 0;}
/*
{
    // Create an empty robust model with T + 1 periods for the RSFC problem
    ROCPPOptModelIF_Ptr RSFCModel(new ROCPPDetOptModel());
    
    // Create decision variables
    ROCPPVarIF_Ptr x1(new ROCPPStaticVarBool("x1",0.));
    ROCPPVarIF_Ptr x2(new ROCPPStaticVarInt("x2",0.));
    
    RSFCModel->add_constraint(x1 + 2.0 * x2 <= 1.0);
    RSFCModel->add_constraint(2.0*x1 + 3.0 * x2 <= 2.0);

    
    // Create a vector that will contain all the elements of the norm term
    vector<ROCPPExpr_Ptr> EllipsoidElements;
    // Populate the vector with the difference between the demand and the nominal demand
    EllipsoidElements.push_back(1.0*x1);
    EllipsoidElements.push_back(1.0*x2);
    // Create the norm term
    shared_ptr<ConstraintTermIF> EllipsoidalConstraintTerm(new NormTerm(EllipsoidElements));
    // Create the ellipsoidal uncertainty constraint
    RSFCModel->add_constraint(EllipsoidalConstraintTerm <= 3.0);
    
    RSFCModel->set_objective(-1.0*x1 - x2); // Add the objective to the problem
    
    // Construct the solver; in this case, use the gurobi solver as a deterministic solver
    SolverParams sparams = SolverParams();
#ifdef USE_GUROBI
    ROCPPSolver_Ptr pSolver(new GurobiModeller(sparams, false) );
#elif defined(USE_SCIP)
    ROCPPSolver_Ptr pSolver(new SCIPModeller(sparams, false) );
#else
                throw MyException("Can not find your solver.");
#endif
    ROCPPMISOCP_Ptr m(convertToMISOCP(RSFCModel));
    // Solve the problem
    pSolver->solve(m);
    
    // Retrieve the optimal solution from the solver
    map<string,double> optimalSln(pSolver->getSolution());
    
    return 0;
    
}
*/
/*
int main(int argc, const char * argv[]) {
 
     uint T(12);
     double InitInventory(0.);
     double InitCommit(100.);
     double NomDemand(100.);
     double rho(0.1);
     double CostDPp(10.);
     double CostDPm(10.);
     double CostDCp(10.);
     double CostDCm(10.);
     
     map<uint,double> OrderUB;
     map<uint,double> OrderLB;
     map<uint,double> HoldingCost;
     map<uint,double> ShortageCost;
     map<uint,double> OrderCost;
     for (uint t=1; t<=T; t++)
     {
         OrderLB[t]=0.;
         OrderUB[t]=200.;
         OrderCost[t]=10.;
         ShortageCost[t+1]=10.;
         HoldingCost[t+1]=2.;
     }
     
     map<uint,double> CumOrderUB;
     map<uint,double> CumOrderLB;
     for (uint t=1; t<=T; t++)
     {
         CumOrderLB[t]=0.;
         CumOrderUB[t]=200.0*t;
     }
     
     // Set "box" here to use box uncertainty set
     string uncertaintySetType("ellipsoidal");

     // Safety parameter for ellipsoidal uncertainty
     double Omega(3.);
     Omega = Omega*NomDemand*rho;
     
     // Create an empty robust model with T + 1 periods for the RSFC problem
     ROCPPOptModelIF_Ptr RSFCModel(new ROCPPUncOptModel(T+1, robust));
     
     // Create the Demand map to store the uncertain parameters of the problem
     map<uint,ROCPPUnc_Ptr> Demand;
     // Iterate over all time periods when there is uncertainty
     for (uint t=1; t<=T; t++)
         // Create the uncertain parameters, and add them to Demand
         Demand[t] = ROCPPUnc_Ptr(new ROCPPUnc("Demand_"+to_string(t),t+1));
     
     
     // Create maps to store the decision variables of the problem
     map<uint,ROCPPVarIF_Ptr> Orders, Commits;    // Quantity ordered, Commitments made
     // Iterate over all time periods from 1 to T
     for (uint t=1; t<=T; t++) {
         // Create the commitment variables (these are static)
         Commits[t]=ROCPPVarIF_Ptr(new ROCPPStaticVarReal("Commit_"+to_string(t),0.));
         if (t==1) // In the first period, the order variables are static
             Orders[t] = ROCPPVarIF_Ptr(new ROCPPStaticVarReal("Order_"+to_string(t),OrderLB[t],OrderUB[t]));
         else // In the other periods, the order variables are adaptive
             Orders[t] = ROCPPVarIF_Ptr(new ROCPPAdaptVarReal("Order_"+to_string(t),t,OrderLB[t],OrderUB[t]));
     }
     
     map<uint,ROCPPVarIF_Ptr> MaxDC; // Upper bound on deviation between commitments
     map<uint,ROCPPVarIF_Ptr> MaxDP; // Upper bound on deviation from plan
     map<uint,ROCPPVarIF_Ptr> MaxHS; // Upper bound on holding and shortage costs
     // Iterate over all time periods 1 to T
     for (uint t=1; t<=T; t++) {
         // Create upper bounds on the deviation between successive commitments
         MaxDC[t] = ROCPPVarIF_Ptr(new ROCPPStaticVarReal("MaxDC_"+to_string(t)));
         // Create upper bounds on the deviation of orders from commitments
         if (t==1)   // In the first period, these are static
             MaxDP[t] = ROCPPVarIF_Ptr(new ROCPPStaticVarReal("MaxDP_"+to_string(t)));
         else        // In the other periods, these are adaptive
             MaxDP[t] = ROCPPVarIF_Ptr(new ROCPPAdaptVarReal("MaxDP_"+to_string(t),t));
         // Create upper bounds on holding and shortage costs (these are adaptive)
         MaxHS[t+1]=ROCPPVarIF_Ptr(new ROCPPAdaptVarReal("MaxHS_"+to_string(t+1),t+1));
     }
     
     // Create the constraints of the problem
     // Create an expression for the amount of inventory held and initialize it
     ROCPPExpr_Ptr Inventory(new ROCPPExpr());
     Inventory = Inventory + InitInventory;
     // Create an expression for the cumulative amount ordered
     ROCPPExpr_Ptr CumOrders(new ROCPPExpr());
     // Iterate over all time periods and add the constraints to the problem
     for (uint t=1; t<=T; t++) {
         // Create the upper and lower bounds on the cumulative orders
         CumOrders = CumOrders + Orders[t];
         RSFCModel->add_constraint(CumOrders >= CumOrderLB[t]);
         RSFCModel->add_constraint(CumOrders <= CumOrderUB[t]);
         // Create upper bound on deviations from commitments
         RSFCModel->add_constraint(MaxDP[t] >= CostDPp*(Orders[t]-Commits[t]));
         RSFCModel->add_constraint(MaxDP[t] >= -CostDPm*(Orders[t]-Commits[t]));
         // Create upper bound on deviations between commitments
         if (t==1) {
             RSFCModel->add_constraint(MaxDC[t] >= CostDCp*(Commits[t]-InitCommit));
             RSFCModel->add_constraint(MaxDC[t] >= -CostDCm*(Commits[t]-InitCommit));
         }
         else {
             RSFCModel->add_constraint(MaxDC[t] >= CostDCp*(Commits[t]-Commits[t-1]));
             RSFCModel->add_constraint(MaxDC[t] >= -CostDCm*(Commits[t]-Commits[t-1]));
         }
         // Update the inventory
         Inventory = Inventory + Orders[t] - Demand[t];
         // Create upper bound on shortage/holding costs
         RSFCModel->add_constraint(MaxHS[t+1] >= HoldingCost[t+1]*Inventory);
         RSFCModel->add_constraint(MaxHS[t+1] >= (-1.*ShortageCost[t+1]*Inventory));
     }
     
     // Create an expression that will contain the objective function
     ROCPPExpr_Ptr RSFCObj(new ROCPPExpr());
     // Iterate over all periods and add the terms to the objective function
     for (uint t=1; t<=T; t++) {
         RSFCObj = RSFCObj + OrderCost[t] * Orders[t] + MaxDC[t] + MaxDP[t] + MaxHS[t+1];
     }
     RSFCModel->set_objective(RSFCObj); // Add the objective to the problem
     
     
     // Create the uncertainty set
     if (uncertaintySetType=="ellipsoidal")
     {
         // Create a vector that will contain all the elements of the norm term
         vector<ROCPPExpr_Ptr> EllipsoidElements;
         // Populate the vector with the difference between the demand and the nominal demand
         for (uint t=1; t<=T; t++)
             EllipsoidElements.push_back(Demand[t] - NomDemand);
         // Create the norm term
         shared_ptr<ConstraintTermIF> EllipsoidalConstraintTerm(new NormTerm(EllipsoidElements));
         // Create the ellipsoidal uncertainty constraint
         RSFCModel->add_constraint_uncset(EllipsoidalConstraintTerm <= Omega);
         
     }
     else if (uncertaintySetType=="box")
     {
         for (uint t=1; t<=T; t++) {
             // Add the upper and lower bounds on the demand to the uncertainty set
             RSFCModel->add_constraint_uncset(Demand[t] >= NomDemand*(1.0-rho));
             RSFCModel->add_constraint_uncset(Demand[t] <= NomDemand*(1.0+rho));
         }
     }

     // Construct the linear/constant decision rule approximator
     ROCPPApproximator_Ptr pLDRApprox(new ROCPPLCDRApprox(RSFCModel));
     // Approximate the adaptive decisions using the linear/constant decision rule approximator and robustify
     ROCPPMISOCP_Ptr RSFCModelLDR(pLDRApprox->approx(RSFCModel));
#ifdef USE_SCIP
    // Construct the solver; in this case, use the gurobi solver as a deterministic solver
    ROCPPSolver_Ptr pSolver(new ROCPPSCIP(SolverParams()));
#elif defined(USE_GUROBI)
    ROCPPSolver_Ptr pSolver(new ROCPPGurobi(SolverParams()));
#endif
     // Solve the problem
     pSolver->solve(RSFCModelLDR);
     
     // Retrieve the optimal solution from the solver
     map<string,double> optimalSln(pSolver->getSolution());
     // Print the optimal decision (from the original model)
     // Prints decision rules from the original problem automatically
     pLDRApprox->printOut(RSFCModelLDR, optimalSln, RSFCModel->getVar("Order_10"));
     
     // Get the optimal objective value
     double optVal(pSolver->getOptValue());
     
     cout << std::setprecision(8) << optVal << endl;

     return 0;
}
*/
 
/*
int main(int argc, const char * argv[])
{
    
    uint T(4);
    uint I(5);
    double B(163.0);
    double theta(1.0);

    map<uint, double> CostUB;
    CostUB[1]=48.;CostUB[2]=86.;CostUB[3]=55.;CostUB[4]=37.;CostUB[5]=30.;

    map<uint, double> ValueUB;
    ValueUB[1]=1030.;ValueUB[2]=1585.;ValueUB[3]=971.;ValueUB[4]=971.;ValueUB[5]=1694.;

    map<uint, double> obsCost;
    for (uint i = 1; i <= I; i++)
    {
        for (uint t = 1; t <= T; t++){
            obsCost.insert(make_pair(t, 0.));
        }
    }
    
    // Create an empty stochastic model with T periods for the BB problem
    ROCPPOptModelIF_Ptr BBModel(new ROCPPDDUOptModel(T, stochastic));

    map<uint, ROCPPUnc_Ptr> Value, Cost;
    for (uint i = 1; i <= I; i++) {
        // Create the value and cost uncertainties associated with box i
        Value[i] = ROCPPUnc_Ptr(new ROCPPUnc("Value_"+to_string(i)));
        Cost[i] = ROCPPUnc_Ptr(new ROCPPUnc("Cost_"+to_string(i)));
    }

    // Create the measurement decisions and pair the uncertain parameters
    map<uint, map<uint, ROCPPVarIF_Ptr> > MVcost, MVval;
    for (uint i = 1; i <= I; i++) {
        // Create the measurement variables associated with the value of box i
        BBModel->add_ddu(Value[i], 1, T, obsCost);
        // Create the measurement variables associated with the cost of box i
        BBModel->add_ddu(Cost[i], 1, T, obsCost);
        // Get the measurement variables and store them in MVval and MVcost
        for (uint t = 1; t <= T; t++) {
            MVval[t][i] = BBModel->getMeasVar(Value[i]->getName(), t);
            MVcost[t][i] = BBModel->getMeasVar(Cost[i]->getName(), t);
        }
    }

    // Pair the uncertain parameters to ensure they are observed at the same time
    for (uint i = 1; i <= I; i++)
        BBModel->pair_uncertainties(Value[i], Cost[i]);

    // Create the keep decisions
    map<uint, map<uint, ROCPPVarIF_Ptr> > Keep;
    for (uint t = 1; t <= T; t++) {
        for (uint i = 1; i <= I; i++) {
            if (t == 1)  // In the first period, the Keep variables are static
                Keep[t][i] = ROCPPVarIF_Ptr(new ROCPPStaticVarBool("Keep_"+to_string(t)+"_"+to_string(i)));
            else   // In the other periods, the Keep variables are adaptive
                Keep[t][i] = ROCPPVarIF_Ptr(new ROCPPAdaptVarBool("Keep_"+to_string(t)+"_"+to_string(i), t));
        }
    }

    // Create the constraints and add them to the problem
    ROCPPExpr_Ptr StoppedSearch(new ROCPPExpr());
    for (uint t = 1; t <= T; t++) {
        // Create the constraint that at most one box be opened at t (none if the search has stopped)
        ROCPPExpr_Ptr NumOpened(new ROCPPExpr());
        // Update the expressions and and the constraint to the problem
        for (uint i = 1; i <= I; i++) {
            StoppedSearch = StoppedSearch + Keep[t][i];
            if (t>1)
                NumOpened = NumOpened + MVval[t][i] - MVval[t-1][i];
            else
                NumOpened = NumOpened + MVval[t][i];
        }
        BBModel->add_constraint( NumOpened <= 1. - StoppedSearch );
        // Constraint that only one of the open boxes can be kept
        for (uint i = 1; i <= I; i++)
            BBModel->add_constraint( (t>1) ? (Keep[t][i] <= MVval[t-1][i]) : (Keep[t][i] <= 0.));
    }

    // Constraint on the amount spent
    ROCPPExpr_Ptr AmountSpent(new ROCPPExpr());
    for (uint i = 1; i <= I; i++)
        AmountSpent = AmountSpent + Cost[i] * MVval[T][i];
    BBModel->add_constraint(AmountSpent <= B);
    
    // Create the uncertainty set constraints and add them to the problem
    for (uint i = 1; i <= I; i++) {
        // Add the upper and lower bounds on the values
        BBModel->add_constraint_uncset(Value[i] >= 0.);
        BBModel->add_constraint_uncset(Value[i] <= ValueUB[i]);
        // Add the upper and lower bounds on the costs
        BBModel->add_constraint_uncset(Cost[i] >= 0.);
        BBModel->add_constraint_uncset(Cost[i] <= CostUB[i]);
    }

    // Create the objective function expression
    ROCPPExpr_Ptr BBObj(new ROCPPExpr());
    for (uint t = 1; t <= T; t++)
        for (uint i = 1; i <= I; i++)
            BBObj = BBObj + pow(theta,t-1)*Value[i]*Keep[t][i];

    // Set objective (multiply by -1 for maximization)
    BBModel->set_objective(-1.0*BBObj);

    // Build the map containing the breakpoint configuration
    map<string,uint> BPconfig;
    BPconfig["Value_1"] = 3;
    BPconfig["Value_2"] = 3;
    BPconfig["Value_4"] = 3;

    // Construct the PWC decision rule approximator
    ROCPPApproximator_Ptr pPWCApprox(new ROCPPPiecewiseApprox(BBModel,BPconfig));
    // Approximate the decisions using PWC decision rules and robustify
    ROCPPMISOCP_Ptr BBModelPWC(pPWCApprox->approx(BBModel));
#ifdef USE_SCIP
    // Construct the solver; in this case, use the gurobi solver as a deterministic solver
    ROCPPSolver_Ptr pSolver(new ROCPPSCIP(SolverParams()));
#elif defined(USE_GUROBI)
    ROCPPSolver_Ptr pSolver(new ROCPPGurobi(SolverParams()));
#endif
    // Solve the problem
    pSolver->solve(BBModelPWC);
    // Retrieve the optimal solution from the solver
    map<string,double> optimalSln(pSolver->getSolution());
    // Print the optimal decision (from the original model)
    // Prints decision rules from the original problem automatically
    pPWCApprox->printOut(BBModelPWC, optimalSln, Keep[4][1]);
    pPWCApprox->printOut(BBModel, optimalSln, Value[4]);

    return 0;
}
*/

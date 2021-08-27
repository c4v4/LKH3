#include "LKH.h"
#include "Segment.h"

/**
 * Modified Penalty function for CVRP instances.
 * The basic idea behind this optimization is to exploit the fact
 * that at each call only few routes (aka petals) have been
 * modified, and we can retrieve this routes simply by looking
 * at the saved Opt moves in the SwapStack array.
 * The implementantio becames less general (since not all the
 * checked tour are obtained from Opt moves), so in some
 * special (and luckily few) cases the old penalty is still called.
 */

#ifdef CAVA_PENALTY

GainType Penalty_CVRP_Old();
static void update_Penalty_CVRP();
static GainType oldPenaltySum;
static int setup_Node_CVRP(Node*);
static int setup_Penalty_CVRP();

GainType Penalty_CVRP()
{
    GainType P = 0;
    if (Swaps && cava_PetalsData)
    {
        GainType DemandSum;
        Node *N;
        
        //Moves that only touch one route cannot change the penalty value
        if (setup_Penalty_CVRP() == 1) 
            return CurrentPenalty;

        for (SwapRecord* si = SwapStack + Swaps - 1; si >= SwapStack; --si)
            for (int twice = 0; twice < 2; ++twice)
            {
                if (twice > 0)
                    N = si->t2->PFlag ? si->t2 : si->t3;
                else
                    N = si->t1->PFlag ? si->t1 : si->t4;

                if (N->PFlag)
                {
                    DemandSum = N->Demand;
                    Node *savedN = N;
                    N->PFlag = 0;
                    
                    //Forward
                    while ((N = SUC(N))->DepotId == 0)
                    {
                        N->PFlag = 0;
                        DemandSum += N->Demand;
                    }

                    GainType tempP = P + DemandSum - Capacity;
                    if (DemandSum > Capacity &&
                        (tempP > oldPenaltySum ||
                        (tempP == oldPenaltySum && CurrentGain <= 0)))
                    { 
                        for (SwapRecord* s = si - 1; s >= SwapStack; --s)
                            s->t1->PFlag = s->t2->PFlag = s->t3->PFlag = s->t4->PFlag = 0;
                        
                        return CurrentPenalty + (CurrentGain > 0);
                    }

                    //Backward
                    N = savedN;
                    while ((N = PRED(N))->DepotId == 0)
                    {
                        N->PFlag = 0;
                        DemandSum += N->Demand;
                    }

                    if (DemandSum > Capacity &&
                        ((P += DemandSum - Capacity) > oldPenaltySum ||
                        (P == oldPenaltySum && CurrentGain <= 0)))
                    { 
                        for (SwapRecord* s = si - 1; s >= SwapStack; --s)
                            s->t1->PFlag = s->t2->PFlag = s->t3->PFlag = s->t4->PFlag = 0;

                        return CurrentPenalty + (CurrentGain > 0);
                    }
                    
                }
            }

        if (!CurrentPenalty)
            return P;

        if (P < oldPenaltySum ||
            (P == oldPenaltySum && CurrentGain > 0))
        { 
            update_Penalty_CVRP(); //Improved!
            return CurrentPenalty + P - oldPenaltySum;
        }
        else
            return CurrentPenalty + (CurrentGain > 0);
    }
    else
    {
        P = Penalty_CVRP_Old();
        if (P < CurrentPenalty ||
            (P == CurrentPenalty && CurrentGain > 0))
        {
            if (!cava_PetalsData)
                cava_PetalsData = (RouteData *)calloc(Salesmen + 1, sizeof(RouteData));
            update_Penalty_CVRP();
        }
        return P;
    }
}

/* Returns 1 if only one route is involved in the current move */
static int setup_Penalty_CVRP()
{
    oldPenaltySum = 0;
    if (CurrentPenalty)
    {
        int petalCounter = 0;
        for (SwapRecord* s = SwapStack + Swaps - 1; s >= SwapStack; --s)
        { 
            //If a Depot->Depot edge is found, an additional empty petal needs to be counted
            petalCounter += (s->t1->DepotId && s->t2->DepotId) || 
                            (s->t3->DepotId && s->t4->DepotId);
            
            petalCounter += setup_Node_CVRP(s->t1) + setup_Node_CVRP(s->t2) +
                            setup_Node_CVRP(s->t3) + setup_Node_CVRP(s->t4);
        }
        
        //Reset petals flags for next petal counting
        for (SwapRecord* s = SwapStack + Swaps - 1; s >= SwapStack; --s)
        {
            s->t1->PetalId->flag = 0;
            s->t2->PetalId->flag = 0;
            s->t3->PetalId->flag = 0;
            s->t4->PetalId->flag = 0;
        }

        if (petalCounter == 1) return 1; //early return
    }

    for (SwapRecord* s = SwapStack + Swaps - 1; s >= SwapStack; --s)
    {
        s->t1->PFlag = !s->t1->DepotId;
        s->t2->PFlag = !s->t2->DepotId;
        s->t3->PFlag = !s->t3->DepotId;
        s->t4->PFlag = !s->t4->DepotId;
    }

    return 0;
}

static int setup_Node_CVRP(Node* N)
{
    if (!N->PetalId->flag)                       
    {                                                            
        oldPenaltySum += N->PetalId->OldPenalty; 
        N->PetalId->flag = 1;                    
        return (N->PetalId != cava_PetalsData); //Depots have PetalId_index == 0                       
    }

    return 0;
}

/* Update route data when a new improving tour is found */
static void update_Penalty_CVRP()
{
    Node *N = Depot;
    RouteData* CurrId;
    GainType DemandSum;
    do
    {
        DemandSum = 0;
        N->PetalId = cava_PetalsData; //depots point to 0 cell
        CurrId = cava_PetalsData + N->DepotId;
        while ((N = SUCC(N))->DepotId == 0)
        {
            N->PetalId = CurrId;
            DemandSum += N->Demand;
        }
        CurrId->OldPenalty = DemandSum > Capacity ? DemandSum - Capacity : 0;
    } while (N != Depot);
}


GainType Penalty_CVRP_Old()
#else
GainType Penalty_CVRP()
#endif
{
    static Node *StartRoute = 0;
    Node *N, *CurrentRoute;
    GainType DemandSum, DistanceSum, P = 0;

    if (!StartRoute)
        StartRoute = Depot;
    if (StartRoute->Id > DimensionSaved)
        StartRoute -= DimensionSaved;
    N = StartRoute;
    do {
        CurrentRoute = N;
        DemandSum = 0;
        do
            DemandSum += N->Demand;
        while ((N = SUCC(N))->DepotId == 0);
        if (DemandSum > Capacity &&
            ((P += DemandSum - Capacity) > CurrentPenalty ||
             (P == CurrentPenalty && CurrentGain <= 0))) {
            StartRoute = CurrentRoute;
            return CurrentPenalty + (CurrentGain > 0);
        }
        if (DistanceLimit != DBL_MAX) {
            DistanceSum = 0;
            N = CurrentRoute;
            do {
                DistanceSum += (C(N, SUCC(N)) - N->Pi - SUCC(N)->Pi) /
                    Precision;
                if (!N->DepotId)
                    DistanceSum += N->ServiceTime;
            } while ((N = SUCC(N))->DepotId == 0);
            if (DistanceSum > DistanceLimit &&
                ((P += DistanceSum - DistanceLimit) > CurrentPenalty ||
                 (P == CurrentPenalty && CurrentGain <= 0))) {
                StartRoute = CurrentRoute;
                return CurrentPenalty + (CurrentGain > 0);
            }
        }
    } while (N != StartRoute);
    return P;
}
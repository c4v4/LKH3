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
 * 
 * At the beginning of the function a setup phase is needed to compute
 * the previous partial penalty associated to the involved routes.
 * 
 * In this phase, each route is considered only once using the "flag" field 
 * of the RouteData.
 * Since depot-copies are part of two differen routes, when counting the 
 * number of routes they are ignored (to avoid ambiguities). 
 * Indeed, depot-copies are not assigned to any real route. They all point 
 * to the same "dummy" RouteData (at index 0 or RouteData array).
 * Empty routes are a special case. In this case, to recognize what
 * originally was an empty route (recall that we only have the
 * proposed solution, the previous one needs to be restored, so
 * we cannot have it), in RouteData is stored the depot pair
 * representing the empty route.
 * 
 * After the setup phase, the actual check starts.
 * As starting point for the penalty computation, only the non-depot nodes 
 * of the current r-Opt move in the SwapStack are considered. Each 
 * non-depot node of the move is marked using the PFlag.
 * In this way, before starting to iterate over a route we can verify if that 
 * routed has been alredy been checked. When a unexplored route is found
 * all the PFlag of its nodes are set to 0 again to avoid multiple checks 
 * for the same route.
 * 
 * The route iteration proceed from the starting node forward until a 
 * depot-copy is found. Then it start again from the starting node 
 * and proceed backward, again untile a depot-copy is found.
 * 
 * When an improvement is found, the update function is called.
 * This function simply iterates over the full tour, updateding each 
 * RouteData value. The index of each route is given by the DepotId of 
 * the depot-copy at the start of that route following the current 
 * tour orientation (which go from 1 to SALESMEN).
 * In this way, every node of the the tour has its own route associated,
 * with its previous penalty.
 */
//#define REDUNDANT_CHECK /* ONLY DEBUG: checks old and new and assert they are the same. */
#ifdef CAVA_PENALTY

#define ARE_LINKED(N1, N2) (N1->Suc == N2 || N1->Pred == N2)

static GainType oldPenaltySum;
GainType Penalty_CVRP_Old(void);            /* Original O(N) Penalty function*/
static void update_Penalty_CVRP(void);      /* O(N) update step called if an improvement is found to updated all route metadata.*/
static int setup_Node_CVRP(Node *);         /* Utility function: count the old penalty sum given the route of the node.*/
static int setup_Penalty_CVRP(void);        /* Compute the previous penalty sum. */
static int was_empty_route(Node *, Node *); /* Used to test *only once* if a removed edge (from a 2opt move) was the edge of an empty 
                                                route (depot->depot). After the first check, it return 0 until the relative
                                                flags is reseted (To avoid counting multiple times the same route).*/

#ifdef REDUNDANT_CHECK
static GainType Penalty_CVRP_();
GainType Penalty_CVRP()
{
    assert(CurrentPenalty >= 0);
    GainType P1 = Penalty_CVRP_();
    GainType P2 = Penalty_CVRP_Old();
    int accepted1 = P1 < CurrentPenalty || (P1 == CurrentPenalty && CurrentGain > 0);
    int accepted2 = P2 < CurrentPenalty || (P2 == CurrentPenalty && CurrentGain > 0);
    assert(accepted1 == accepted2);
    assert(P1 >= 0);
    return P1;
}

GainType Penalty_CVRP_()
#else
GainType Penalty_CVRP()
#endif
{
    GainType P = 0;
    if (Swaps && cava_PetalsData)
    {
        GainType DemandSum;
        Node *N;
        //Moves that only touch one route cannot change the penalty value
        if (setup_Penalty_CVRP() == 1)
            return CurrentPenalty;
        for (SwapRecord *si = SwapStack + Swaps - 1; si >= SwapStack; --si)
        {
            // Test for empty routes
            if ((si->t1->DepotId && si->t4->DepotId && ARE_LINKED(si->t1, si->t4)) ||
                (si->t2->DepotId && si->t3->DepotId && ARE_LINKED(si->t2, si->t3)))
                P += MTSPMinSize;
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
                    int Size = 1;
                    N->PFlag = 0;
                    //Forward
                    while ((N = SUC(N))->DepotId == 0)
                    {
                        N->PFlag = 0;
                        DemandSum += N->Demand;
                        ++Size;
                    }
                    GainType tempP = P + DemandSum - Capacity;
                    if (DemandSum > Capacity &&
                        (tempP > oldPenaltySum ||
                         (tempP == oldPenaltySum && CurrentGain <= 0)))
                    {
                        for (SwapRecord *s = si - 1; s >= SwapStack; --s)
                            s->t1->PFlag = s->t2->PFlag = s->t3->PFlag = s->t4->PFlag = 0;

                        return CurrentPenalty + (CurrentGain > 0);
                    }
                    //Backward
                    N = savedN;
                    while ((N = PRED(N))->DepotId == 0)
                    {
                        N->PFlag = 0;
                        DemandSum += N->Demand;
                        ++Size;
                    }
                    if (Size < MTSPMinSize)
                        P += MTSPMinSize - Size;

                    if (DemandSum > Capacity &&
                        ((P += DemandSum - Capacity) > oldPenaltySum ||
                         (P == oldPenaltySum && CurrentGain <= 0)))
                    {
                        for (SwapRecord *s = si - 1; s >= SwapStack; --s)
                            s->t1->PFlag = s->t2->PFlag = s->t3->PFlag = s->t4->PFlag = 0;

                        return CurrentPenalty + (CurrentGain > 0);
                    }
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
    int petalCounter = 0;
    if (CurrentPenalty)
    {
        for (SwapRecord *s = SwapStack + Swaps - 1; s >= SwapStack; --s)
        {
            //If a move has involved the edge of an empty route an additional empty one needs to be counted
            Node *t1 = s->t1, *t2 = s->t2, *t3 = s->t3, *t4 = s->t4;

            if ((!ARE_LINKED(t1, t2) && was_empty_route(t1, t2)) ||
                (!ARE_LINKED(t3, t4) && was_empty_route(t3, t4)))
            {
                ++petalCounter;
                oldPenaltySum += MTSPMinSize;
            }
            petalCounter += setup_Node_CVRP(t1) + setup_Node_CVRP(t2) +
                            setup_Node_CVRP(t3) + setup_Node_CVRP(t4);
        }
        //Reset petals flags for next petal counting
        for (SwapRecord *s = SwapStack + Swaps - 1; s >= SwapStack; --s)
        {
            Node *t1 = s->t1, *t2 = s->t2, *t3 = s->t3, *t4 = s->t4;
            int d1 = t1->DepotId, d2 = t2->DepotId, d3 = t3->DepotId, d4 = t4->DepotId;

            cava_PetalsData[d1].flag = cava_PetalsData[d2].flag =
                cava_PetalsData[d3].flag = cava_PetalsData[d4].flag = 0;

            t1->PetalId->flag = t2->PetalId->flag = t3->PetalId->flag = t4->PetalId->flag = 0;
        }
        if (petalCounter == 1)
            return 1;
    }
    for (SwapRecord *s = SwapStack + Swaps - 1; s >= SwapStack; --s)
    {
        s->t1->PFlag = !s->t1->DepotId;
        s->t2->PFlag = !s->t2->DepotId;
        s->t3->PFlag = !s->t3->DepotId;
        s->t4->PFlag = !s->t4->DepotId;
    }
    return petalCounter;
}

static int was_empty_route(Node *N1, Node *N2)
{
    int *f1 = &cava_PetalsData[N1->DepotId].flag;
    int *f2 = &cava_PetalsData[N2->DepotId].flag;
    return (!*f1 && (*f1 |= (cava_PetalsData[N1->DepotId].minNode == N2))) ||
           (!*f2 && (*f2 |= (cava_PetalsData[N2->DepotId].minNode == N1)));
}

static int setup_Node_CVRP(Node *N)
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
    RouteData *CurrId;
    GainType DemandSum;
    int Size;
    do
    {
        DemandSum = Size = 0;
        N->PetalId = cava_PetalsData; //depots point to 0 cell
        CurrId = cava_PetalsData + N->DepotId;
        while ((N = SUCC(N))->DepotId == 0)
        {
            ++Size;
            N->PetalId = CurrId;
            DemandSum += N->Demand;
        }
        CurrId->OldPenalty = DemandSum > Capacity ? DemandSum - Capacity : 0;
        CurrId->OldPenalty += Size < MTSPMinSize ? MTSPMinSize - Size : 0;
        CurrId->minNode = Size ? NULL : N; /*Save the adjacent depot to recognize empty routes*/
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
    int Size;
    if (!StartRoute)
        StartRoute = Depot;
    if (StartRoute->Id > DimensionSaved)
        StartRoute -= DimensionSaved;
    N = StartRoute;
    do
    {
        CurrentRoute = N;
        DemandSum = 0;
        Size = -1;
        do
        {
            ++Size;
            DemandSum += N->Demand;
        } while ((N = SUCC(N))->DepotId == 0);
        if (Size < MTSPMinSize)
            P += MTSPMinSize - Size;
        if (DemandSum > Capacity &&
            ((P += DemandSum - Capacity) > CurrentPenalty ||
             (P == CurrentPenalty && CurrentGain <= 0)))
        {
            StartRoute = CurrentRoute;
            return CurrentPenalty + (CurrentGain > 0);
        }
        if (DistanceLimit != DBL_MAX)
        {
            DistanceSum = 0;
            N = CurrentRoute;
            do
            {
                DistanceSum += (C(N, SUCC(N)) - N->Pi - SUCC(N)->Pi) /
                               Precision;
                if (!N->DepotId)
                    DistanceSum += N->ServiceTime;
            } while ((N = SUCC(N))->DepotId == 0);
            if (DistanceSum > DistanceLimit &&
                ((P += DistanceSum - DistanceLimit) > CurrentPenalty ||
                 (P == CurrentPenalty && CurrentGain <= 0)))
            {
                StartRoute = CurrentRoute;
                return CurrentPenalty + (CurrentGain > 0);
            }
        }
    } while (N != StartRoute);
    return P;
}
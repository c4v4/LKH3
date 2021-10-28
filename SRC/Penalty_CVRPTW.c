#include "LKH.h"
#include "Segment.h"

/**
 * Modified Penlaty function for CVRPTW instances.
 * The basic idea behind this optimization is to exploit the fact
 * that at each call only few "routes" (aka petals) have been
 * modified, and we can retrieve this petals simply by looking
 * at the saved Opt-moves in the SwapStack array.
 * The implementantio becames less general (since not all the
 * checked tour are obtained from Opt moves), so in some
 * special (and few) cases the old penalty is still called.
 * 
 * Differently form the CVRP Penalty, in this case the asymmetric 
 * representation of the solution require some further adaptations.
 * 
 * At the beginning of the function a setup phase is needed to compute
 * the previous partial penalty associated to the involved routes.
 * 
 * In this phase:
 * - The old partial penalty is computed.
 * - The depot-copy at the start of the routes is stored in 
 *   cava_NodeCache.
 * - The minNode of each route is saved to further speeding up the 
 *   route explorarion.
 * 
 * Each route is considered only once using the "flag" field of the 
 * RouteData.
 * **Differently from the CVRP penalty case**, here each depot copy 
 * is considered as part of the route that follows in the tour 
 * representation.
 * Moreover the field RouteData::PetalRank is used to find which is 
 * the first node (minNode) of the route touched by the current r-Opt 
 * move. 
 * It will be used later to skip the initial part of the route. 
 * (NOTE: the same could have been made for the last node, however the 
 * overhead introduced by that change has shown to be greater of the 
 * actual improvement).
 *  
 * After the setup phase, the actual check starts.
 * The depot-copies saved in the cava_NodeCache are used to retrieve 
 * the RouteData information. The minNode tell the starting point, 
 * while the minNode->prevCostSum, minNode->prevDemandSum and  
 * minNode->prevPenalty fileds are used to initialize the relative 
 * variables (CostSum, DemandSum and petalP), warmstarting the route
 * exploration.
 * 
 * The route iteration proceed only forward in this case, until an 
 * early-exit condition, or a depot-copy is found.
 * 
 * When an improvement is found, the update function is called.
 * The cava_NodeCache is used to rapidly retrieve which routes need to 
 * be updated.
 * In this case a more complex update procedure needs to be 
 * accomplished, since, along with the new partial penalty of the route, 
 * also the Node::prevCostSum, Node::prevDemandSum and 
 * Node::prevPenalty fileds of each node of the route need to be updated.
 */

//#define REDUNDANT_CHECK /* ONLY DEBUG: checks old and new and assert they are the same. */
#ifdef CAVA_PENALTY

GainType Penalty_CVRPTW_Old();
static GainType oldPenaltySum;
static int setup_Penalty_CVRPTW();
static Node *setup_node_CVRPTW(Node *N);
static void route_min_node_update(Node *t);
static void update_Penalty_CVRPTW();

#ifdef REDUNDANT_CHECK
static GainType Penalty_CVRPTW_();
GainType Penalty_CVRPTW()
{
    GainType P1 = Penalty_CVRPTW_();
    GainType P2 = Penalty_CVRPTW_Old();
    int accepted1 = P1 < CurrentPenalty || (P1 == CurrentPenalty && CurrentGain > 0);
    int accepted2 = P2 < CurrentPenalty || (P2 == CurrentPenalty && CurrentGain > 0);
    assert(accepted1 == accepted2);
    return P1;
}

GainType Penalty_CVRPTW_()
#else
GainType Penalty_CVRPTW()
#endif
{
    GainType DemandSum, CostSum, P = 0, petalP;
    Node *N, *NextN;
    int NC_idx, Size;
    int Forward = SUCC(Depot)->Id != Depot->Id + DimensionSaved;

    if (Swaps && cava_PetalsData)
    {
        NC_idx = setup_Penalty_CVRPTW();
        while (NC_idx)
        {
            RouteData *petal = cava_NodeCache[--NC_idx]->PetalId;
            N = petal->minNode;
            if (N->Id > DimensionSaved)
                N -= DimensionSaved;
            Size = (N->PetalRank / 2) - 1;
            CostSum = N->prevCostSum;
            DemandSum = N->prevDemandSum;
            petalP = N->prevPenalty;
            do
            {
                ++Size;
                if ((DemandSum += N->Demand) > Capacity)
                    petalP += DemandSum - Capacity;
                if (CostSum < N->Earliest)
                    CostSum = N->Earliest;
                if (CostSum > N->Latest)
                    petalP += CostSum - N->Latest;
                if ((P + petalP) > oldPenaltySum ||
                    ((P + petalP) == oldPenaltySum && CurrentGain <= 0))
                    return CurrentPenalty + (CurrentGain > 0);
                CostSum += N->ServiceTime;
                NextN = (Forward ? SUC(N) : PRED(N));
                int cost = (Forward ? SUC_COST(N) : PRED_COST(N)) - N->Pi -
                           NextN->Pi;
                CostSum += cost / Precision;
                N = (Forward ? SUC(NextN) : PRED(NextN));
            } while (N->DepotId == 0);
            if (Size < MTSPMinSize)
                petalP += MTSPMinSize - Size;
            if (CostSum > Depot->Latest)
                petalP += CostSum - Depot->Latest;
            if ((P + petalP > oldPenaltySum ||
                 (P + petalP == oldPenaltySum && CurrentGain <= 0)))
                return CurrentPenalty + (CurrentGain > 0);
            P += petalP;
            petal->CandPenalty = petalP;
        }
        if (!CurrentPenalty || P < oldPenaltySum ||
            (P == oldPenaltySum && CurrentGain > 0))
        {
            update_Penalty_CVRPTW();
            return CurrentPenalty + P - oldPenaltySum;
        }
        else
            return CurrentPenalty + (CurrentGain > 0);
    }
    else
    {
        if (!cava_NodeCache)
            cava_NodeCache = (Node **)calloc(Salesmen + 1, sizeof(Node *));
        if (!cava_PetalsData)
            cava_PetalsData = (RouteData *)calloc(Salesmen + 1, sizeof(RouteData));
        P = Penalty_CVRPTW_Old();
        if (P < CurrentPenalty || (P == CurrentPenalty && CurrentGain > 0))
            update_Penalty_CVRPTW();
        return P;
    }
}

int setup_Penalty_CVRPTW()
{
    Node *N;
    int touched_routes = 0;
    oldPenaltySum = 0;
    for (SwapRecord *s = SwapStack + Swaps - 1; s >= SwapStack; --s)
    {
        s->t1->PetalId->flag = s->t4->PetalId->flag = 0;
        s->t1->PetalId->minNode = s->t4->PetalId->minNode = NULL;
    }
    for (SwapRecord *s = SwapStack + Swaps - 1; s >= SwapStack; --s)
    {
        if ((N = setup_node_CVRPTW(s->t1)) != NULL)
            cava_NodeCache[touched_routes++] = N;
        if ((N = setup_node_CVRPTW(s->t4)) != NULL)
            cava_NodeCache[touched_routes++] = N;
        route_min_node_update(s->t1);
        route_min_node_update(s->t2);
        route_min_node_update(s->t3);
        route_min_node_update(s->t4);
    }
    cava_NodeCache[touched_routes] = NULL;
    return touched_routes;
}

void route_min_node_update(Node *t)
{
    if (!t->PetalId->minNode || t->PetalId->minNode->PetalRank > t->PetalRank)
        t->PetalId->minNode = t;
}

Node *setup_node_CVRPTW(Node *N)
{

    if (!N->PetalId->flag)
    {
        oldPenaltySum += N->PetalId->OldPenalty;
        N->PetalId->flag = 1;
        int DepotId = N->PetalId - cava_PetalsData;
        return DepotId == MTSPDepot ? Depot : NodeSet + Dim - 1 + DepotId;
    }
    return NULL;
}

void update_Penalty_CVRPTW()
{
    Node *N, *NextN;
    GainType DemandSum, CostSum, petalP;
    int Forward = SUCC(Depot)->Id != Depot->Id + DimensionSaved;
    for (Node **CN = cava_NodeCache; *CN; ++CN)
    {
        N = *CN;
        if (N->Id > DimensionSaved)
            N -= DimensionSaved;
        RouteData *CurrId = cava_PetalsData + N->DepotId;
        CurrId->OldPenalty = CurrId->CandPenalty;
        DemandSum = CostSum = petalP = 0;
        int PetalRank = 0;
        do
        {
            NextN = Forward ? SUCC(N) : PREDD(N);
            N->PetalId = NextN->PetalId = CurrId;
            N->PetalRank = PetalRank++;
            NextN->PetalRank = PetalRank++;
            N->prevDemandSum = NextN->prevDemandSum = DemandSum;
            N->prevCostSum = NextN->prevCostSum = CostSum;
            N->prevPenalty = NextN->prevPenalty = petalP;
            if ((DemandSum += N->Demand) > Capacity)
                petalP += DemandSum - Capacity;
            if (CostSum < N->Earliest)
                CostSum = N->Earliest;
            if (CostSum > N->Latest)
                petalP += CostSum - N->Latest;
            CostSum += N->ServiceTime;
            CostSum += (C(N, NextN) - N->Pi - NextN->Pi) / Precision;
            N = Forward ? SUCC(NextN) : PREDD(NextN);
        } while (N->DepotId == 0);
    }
}

GainType Penalty_CVRPTW_Old()
{
    static _Thread_local Node *StartRoute = 0;
    Node *N, *NextN, *CurrentRoute;
    GainType CostSum, DemandSum, P = 0, petalP;
    int Forward = SUCC(Depot)->Id != Depot->Id + DimensionSaved;
    int cache_index = 0, Size;
    if (!StartRoute)
        StartRoute = Depot;
    if (StartRoute->Id > DimensionSaved)
        StartRoute -= DimensionSaved;
    N = StartRoute;
    do
    {
        cava_NodeCache[cache_index++] = CurrentRoute = N;
        CostSum = DemandSum = petalP = 0;
        Size = -1; /* Account for depot */
        do
        {
            ++Size;
            if (N->Id <= Dim && N != Depot)
            {
                if ((DemandSum += N->Demand) > Capacity)
                    petalP += DemandSum - Capacity;
                if (CostSum < N->Earliest)
                    CostSum = N->Earliest;
                if (CostSum > N->Latest)
                    petalP += CostSum - N->Latest;
                if ((P + petalP) > CurrentPenalty ||
                    ((P + petalP) == CurrentPenalty && CurrentGain <= 0))
                {
                    StartRoute = CurrentRoute;
                    return CurrentPenalty + (CurrentGain > 0);
                }
                CostSum += N->ServiceTime;
            }
            NextN = Forward ? SUCC(N) : PREDD(N);
            CostSum += (C(N, NextN) - N->Pi - NextN->Pi) / Precision;
            N = Forward ? SUCC(NextN) : PREDD(NextN);
        } while (N->DepotId == 0);
        if (Size < MTSPMinSize)
            petalP += MTSPMinSize - Size;
        if (CostSum > Depot->Latest &&
            ((P + (petalP += CostSum - Depot->Latest)) > CurrentPenalty ||
             ((P + petalP) == CurrentPenalty && CurrentGain <= 0)))
        {
            StartRoute = CurrentRoute;
            return CurrentPenalty + (CurrentGain > 0);
        }
        P += petalP;
        cava_PetalsData[CurrentRoute->DepotId].CandPenalty = petalP;
    } while (N != StartRoute);

    return P;
}

#else

GainType Penalty_CVRPTW()
{
    static _Thread_local Node *StartRoute = 0;
    Node *N, *NextN, *CurrentRoute;
    GainType CostSum, DemandSum, P = 0;
    int Forward = SUCC(Depot)->Id != Depot->Id + DimensionSaved;
    int Size;

    if (!StartRoute)
        StartRoute = Depot;
    if (StartRoute->Id > DimensionSaved)
        StartRoute -= DimensionSaved;
    N = StartRoute;
    do
    {
        CurrentRoute = N;
        CostSum = DemandSum = 0;
        Size = -1;
        do
        {
            ++Size;
            if (N->Id <= Dim && N != Depot)
            {
                if ((DemandSum += N->Demand) > Capacity)
                    P += DemandSum - Capacity;
                if (CostSum < N->Earliest)
                    CostSum = N->Earliest;
                if (CostSum > N->Latest)
                    P += CostSum - N->Latest;
                if (P > CurrentPenalty ||
                    (P == CurrentPenalty && CurrentGain <= 0))
                {
                    StartRoute = CurrentRoute;
                    return CurrentPenalty + (CurrentGain > 0);
                }
                CostSum += N->ServiceTime;
            }
            NextN = Forward ? SUCC(N) : PREDD(N);

            CostSum += (C(N, NextN) - N->Pi - NextN->Pi) / Precision;
            N = Forward ? SUCC(NextN) : PREDD(NextN);
        } while (N->DepotId == 0);
        if (Size < MTSPMinSize)
            petalP += MTSPMinSize - Size;
        if (CostSum > Depot->Latest &&
            ((P += CostSum - Depot->Latest) > CurrentPenalty ||
             (P == CurrentPenalty && CurrentGain <= 0)))
        {
            StartRoute = CurrentRoute;
            return CurrentPenalty + (CurrentGain > 0);
        }
    } while (N != StartRoute);
    return P;
}
#endif
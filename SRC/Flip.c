#include "LKH.h"

#ifdef CAVA_FLIP
static void cava_FlipAsym(Node *t1, Node *t2, Node *t3);
#endif

/*
 * The Flip function performs a 2-opt move. Edges (t1,t2) and (t3,t4) 
 * are exchanged with edges (t2,t3) and (t4,t1). Node t4 is one of 
 * t3's two neighbors on the tour; which one is uniquely determined
 * by the orientation of (t1,t2).
 *
 * The function is only used if the doubly linked list representation 
 * is used for a tour; if the two-level tree representation is used, 
 * the function Flip_SL is used instead.
 *
 * The 2-opt move is made by swapping Pred and Suc of each node of the
 * two segments, and then reconnecting the segments by suitable
 * settings of Pred and Suc of t1, t2, t3 and t4. In addition,
 * Rank is updated for nodes in the reversed segment (Rank gives the
 * ordinal number of a node in the tour).
 *
 * Any of two segments defined by the 2-opt move may be reversed. The
 * segment with the smallest number of nodes is reversed in order to
 * speed up computations. The number of nodes in a segment is found 
 * from the Rank-values. 
 * 
 * The move is pushed onto a stack of 2-opt moves. The stack makes it
 * possible to undo moves (by the RestoreTour function).
 *
 * Finally, the hash value corresponding to the tour is updated. 
 */

void Flip(Node *t1, Node *t2, Node *t3)
{
    
#ifdef CAVA_FLIP
    if (Asymmetric)
    {
        cava_FlipAsym(t1, t2, t3);
        return;
    }
#endif

    Node *s1, *s2, *t4;
    int R, Temp, Ct2t3, Ct4t1;

    assert(t1->Pred == t2 || t1->Suc == t2);
    if (t3 == t2->Pred || t3 == t2->Suc)
        return;
    t4 = t1->Suc == t2 ? t3->Pred : t3->Suc;
    if (t1->Suc != t2) {
        s1 = t1;
        t1 = t2;
        t2 = s1;
        s1 = t3;
        t3 = t4;
        t4 = s1;
    }
    /* Find the segment with the smallest number of nodes */
    if ((R = t2->Rank - t3->Rank) < 0)
        R += Dimension;
    if (2 * R > Dimension) {
        s1 = t3;
        t3 = t2;
        t2 = s1;
        s1 = t4;
        t4 = t1;
        t1 = s1;
    }
    Ct2t3 = C(t2, t3);
    Ct4t1 = C(t4, t1);
    /* Swap segment (t3 --> t1) */
    R = t1->Rank;
    t1->Suc = 0;
    s2 = t3;
    while ((s1 = s2)) {
        s2 = s1->Suc;
        s1->Suc = s1->Pred;
        s1->Pred = s2;
        s1->Rank = R--;
        Temp = s1->SucCost;
        s1->SucCost = s1->PredCost;
        s1->PredCost = Temp;
    }
    (t3->Suc = t2)->Pred = t3;
    (t4->Suc = t1)->Pred = t4;
    t3->SucCost = t2->PredCost = Ct2t3;
    t1->PredCost = t4->SucCost = Ct4t1;
    SwapStack[Swaps].t1 = t1;
    SwapStack[Swaps].t2 = t2;
    SwapStack[Swaps].t3 = t3;
    SwapStack[Swaps].t4 = t4;
    Swaps++;
    Hash ^= (Rand[t1->Id] * Rand[t2->Id]) ^
        (Rand[t3->Id] * Rand[t4->Id]) ^
        (Rand[t2->Id] * Rand[t3->Id]) ^ (Rand[t4->Id] * Rand[t1->Id]);
}

#ifdef CAVA_FLIP
/*
 * Specialized Flip function for asymmetric instances.
 * 
 * The Flip function performs a 2-opt move. Edges (t1,t2) and (t3,t4) 
 * are exchanged with edges (t2,t3) and (t4,t1). Node t4 is one of 
 * t3's two neighbors on the tour; which one is uniquely determined
 * by the orientation of (t1,t2).
 *
 * The function is only used if the doubly linked list representation 
 * is used for a tour; if the two-level tree representation is used, 
 * the function Flip_SL is used instead.
 * 
 * Since asymmetric function do not allow flips, this function apply
 * a 2-opt move without changing the orientation of the segments.
 * 
 * The move is pushed onto a stack of 2-opt moves. The stack makes it
 * possible to undo moves (by the RestoreTour function).
 */

void cava_FlipAsym(Node *t1, Node *t2, Node *t3)
{
    Node *t4;
    int Ct2t3, Ct4t1;

    /* t2 and t3 */
    Ct2t3 = C(t2, t3);
    if (t3->FixedTo1 == t3->Pred)
    {
        t4 = t3->Suc;
        t3->Suc = t2;
        t3->SucCost = Ct2t3;
    }
    else
    {
        t4 = t3->Pred;
        t3->Pred = t2;
        t3->PredCost = Ct2t3;
    }

    if (t2->FixedTo1 == t2->Pred)
    {
        t2->Suc = t3;
        t2->SucCost = Ct2t3;
    }
    else
    {
        t2->Pred = t3;
        t2->PredCost = Ct2t3;
    }

    /* t4 and t1 */
    Ct4t1 = C(t4, t1);
    if (t4->FixedTo1 == t4->Pred)
    {
        t4->Suc = t1;
        t4->SucCost = Ct4t1;
    }
    else
    {
        t4->Pred = t1;
        t4->PredCost = Ct4t1;
    }

    if (t1->FixedTo1 == t1->Pred)
    {
        t1->Suc = t4;
        t1->SucCost = Ct4t1;
    }
    else
    {
        t1->Pred = t4;
        t1->PredCost = Ct4t1;
    }

    SwapStack[Swaps].t1 = t1;
    SwapStack[Swaps].t2 = t2;
    SwapStack[Swaps].t3 = t3;
    SwapStack[Swaps].t4 = t4;
    Swaps++;
}

void cava_FlipAsym_Update()
{
    Node *FirstN = NULL, *LastN = NULL;
    if (Swaps)
    {
        FirstN = LastN = SwapStack[0].t1;
        for (int i = Swaps - 1; i >= 0; --i)
        {
            Node *t1 = SwapStack[i].t1;
            Node *t2 = SwapStack[i].t2;
            Node *t3 = SwapStack[i].t3;
            Node *t4 = SwapStack[i].t4;

            Hash ^= (Rand[t1->Id] * Rand[t2->Id]) ^
                    (Rand[t3->Id] * Rand[t4->Id]) ^
                    (Rand[t2->Id] * Rand[t3->Id]) ^
                    (Rand[t4->Id] * Rand[t1->Id]);

            if (FirstN != FirstNode)
            {
                if (t1 == FirstNode ||
                    t2 == FirstNode ||
                    t3 == FirstNode ||
                    t4 == FirstNode)
                    FirstN = LastN = FirstNode;
                else
                {
                    if (FirstN->Rank > t1->Rank)
                        FirstN = t1;
                    else if (LastN->Rank < t1->Rank)
                        LastN = t1;

                    if (FirstN->Rank > t2->Rank)
                        FirstN = t2;
                    else if (LastN->Rank < t2->Rank)
                        LastN = t2;

                    if (FirstN->Rank > t3->Rank)
                        FirstN = t3;
                    else if (LastN->Rank < t3->Rank)
                        LastN = t3;

                    if (FirstN->Rank > t4->Rank)
                        FirstN = t4;
                    else if (LastN->Rank < t4->Rank)
                        LastN = t4;
                }
            }
        }

        Node *N = FirstN;
        int Rank = N->Rank;
        while ((N = N->Suc) != LastN)
            N->Rank = ++Rank;
    }
}

#endif
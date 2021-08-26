Vehicle routing problem with mixed pickup and delivery (VRPMPD)

The subdirectory INSTANCES contains the following benchmark instances:

	S (70 instances)
	Source:
	Salhi, S., Nagy, G.:
	A Cluster Insertion Heuristic for Single and Multiple Depot Vehicle Routing 
	Problems with Backhauling.
	J. Oper. Res. Soc., 50(10):1034-1042 (1999)

The first integer gives the number of the node.
The second integer gives its demand (ignored).
The third and fourth number give the earliest and latest time for the node.
The fifth number specifies the service time for the node.
The last two integers are used to specify pickup and delivery. The first of
these integers gives the size of the pickup, whereas the second integer gives
the size of the delivery.

The subdirectory TOURS contains the best tours found by LKH-3.

Tabulated results can be found in the subdirectory RESULTS.
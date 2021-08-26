Vehicle routing problem with simultaneous pickup-delivery and time windows (VRPSDTW)

The subdirectory INSTANCES contains the following benchmark instances:

	W (69 instances)
	Source:
	Wang, H.-F., Chen, Y.-Y.:
	A genetic algorithm for the simultaneous delivery and pickup problems
	with time windows.
	Comput. Ind. Eng., 62:84-95, 2012.
	
PICKUP_AND_DELIVERY_SECTION :
Each line is of the form

      <integer> <integer> <real> <real> <real> <integer> <integer>

The first integer gives the number of the node.
The second integer gives its demand (ignored).
The third and fourth number give the earliest and latest time for the node.
The fifth number specifies the service time for the node.
The last two integers are used to specify pickup and delivery. The first of 
these integers gives the size of the pickup, whereas the second integer gives
the size of the delivery.

The subdirectory TOURS contains the best tours found by LKH-3.

Tabulated results can be found in the subdirectory RESULTS.
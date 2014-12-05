dynamic_multi_plugin

Notes on the reverse search
---------------------------

An "arrive before" request works by reverting the graph and "starting" from the required destination and going backward in the graph.

For public transports, things are hard to manipulate, since times must also be thought upside down and wait times are also to be inserted.

In a "normal" forward traversal, when on a PT (source) vertex and considering another PT (destination) vertex,
we look at the timetable to see if a trip will go from the source vertex to the destination vertex.
If we are already on a PT trip, we can choose to go on this trip or get off board and wait for another trip.
Or if we are currently walking, we have to wait for the next bus. A "min_transfer_time" is also considered to
take into account (lack of) scheduling accuray in PT timetables.
So there is a variable "wait time" added.
A wait time is attached to the 'target' vertex of an edge in a path.

In a backward traversal, wait times are harder to manipulate.
Suppose we want to arrive before 17h. We then "start" Dijkstra with the destination vertex and a cost of -17h.
Suppose the current optimal path is on a road, no problem, we normally compute a (positive) cost for each road
edge. Suppose we "arrive" at -16h50 at a bus stop. And we know a bus trip ends on this stop at 16h45.
Then there is a time difference between the "current" time and the time we could be arriving from the bus (5min).
But there would be no sense to include this wait time here. If we included the wait time here, it would give the
following roadmap, when everything will be reverted :

* 16h40 Take the bus trip #X
* 16h45 Leave at stop #Y
*       Wait 5 min (!!)
* 16h50 Walk to destination
* 17h You are arrived

Instead, we want something like :

* 16h40 Take the bus trip #X
* 16h45 Leave at stop #Y
*       Walk to destination
* 16h55 You are arrived

So we introduce a 'shift_time' which means the current time must be shifted a bit.
This 'shift' will be used when reversing the final path.

In order to build the final roadmap, the raw path is then processed in a first step to put wait time at the right place
and get rid of "shift" times.

#CSCI402 Project

This is code from my Final project for Operating Systems (CSCI402) at the University of Southern California. The code is purely here for demonstration purposes.

##Explanation

My responsibility for the project was to create a distributed system which handled client requests in the NachOS operating system.

The distributed system consisted of 5 servers.  These 5 servers we're responsible for receiving client requests, and accessing a database which contained stored information.  

The stored information consisted of Locks, Condition Variables and Monitor Variables. 

The clients were several user programs which all needed to access data from the distributed servers.  Additionally, these client programs needed to execute without race conditions occuring. 

###Constraint
As apart of this project, we we're required to make ***all*** servers process ***all*** client requests. 

##Solution

I ended up using the Total Ordering Algorithm to solve this problem.  The reasoning behind using this algorithm stems from the fact that client requests can be received out of order.  This fact is important because we have multiple servers handling client requests, and if the requests are out of order, the client programs will receive responses out of order. Eventually, the client programs will crash.

So, we enforced the ordering by implementing the ***Total Ordering Algorithm*** like so:

###Tasks

1. When server receives client message, forward a server-style message to all other servers
	- Add info to the message
		- timestamp
		- forwarding server machine ID & mailbox number
2. Total Ordering
	- Data
		1. ***Sorted Queue*** of unprocessed client request messages (ordered by timestamp and forwarding server machine id)
		2. ***Last Timestamp Received*** Table - An array of timestamp values (index/element for each server).
			- store the last timestamp received from each server
			
###Total Ordering Algorithm

1. A server receives the server-forwarded message

2. Once we receive the message, let's extract info from it
	- extract the timestamp and forward message id

3. Put the message in pending message queue in sorted order

4. Update the ***Last Timestamp Received*** table with the timestamp of the new message for the forwarding server's machine ID

5. Scan the LTR table and extract the smallest timestamp
6. Process any msg in our pending message queue that has a timestamp smaller than the one from step 5

We're not done yet :)

###Make sure all messages handled

Due to how we send client requests (client chooses random server), there's a chance that some servers won't receive client messages.  A server that doesn't receive a client message won't have its index in the LTR table updated.  This means that there's a chance it can remain the smallest index, and that newer requests from other servers won't get processed.

Simple solution: Evertime a server receives a server forwarded message, send a timestamp msg to all other servers.

##Implementation

The source code is fairly long.  The cool stuff is in a function called:

	TotalOrder(int numServers){
		//teh codez
	}

I implemented the majority of this code, with help from my teammates as well. It was my first big C++ project :). 
//
//  server.cc
//  
//
//  Created by Nii Mante on 3/14/12.
//  Copyright (c) 2012 University of Southern California. All rights reserved.
//

#include <iostream>
#include "server.h"
#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"

ServerLockEntry serverLockTable[MAX_LOCKS];
ServerConditionEntry serverConditionTable[MAX_CONDITIONS];
ServerMvEntry serverMvTable[MAX_MVS]; 


/*
 * ServerLock class implementations
 */
bool ServerLock::isHeldByMe(int netAddress, int mailBoxNumber){
    if (owner.netAddr == netAddress && owner.mailBoxNum == mailBoxNumber) {
        return true;
    }
    return false;
}

void ServerLock::listAppend(char *msg){
    lockWaitQueue->Append(msg);
}
void ServerLock::listPrepend(char *msg){
    lockWaitQueue->Prepend(msg);
}

void *ServerLock::listRemove(){
    return lockWaitQueue->Remove();
}

bool ServerLock::isEmpty(){
    return lockWaitQueue->IsEmpty();
}

ServerLock::ServerLock(char *myName){
    name = new char[strlen(myName)];
    sprintf(name,"%s",myName);
    lockWaitQueue = new List();
    serverLockState = FREE;
}

ServerLock::~ServerLock(){
    delete [] name;
    name = NULL;
    delete lockWaitQueue;
    lockWaitQueue = NULL;
    serverLockState = BUSY;
    
}

/*
 * ServerCondition class implementations
 */

void ServerCondition::listAppend(char *msg){
    cvWaitQueue->Append(msg);
}

void *ServerCondition::listRemove(){
    return cvWaitQueue->Remove();
}

bool ServerCondition::isEmpty(){
    return cvWaitQueue->IsEmpty();
}

ServerCondition::ServerCondition(char *myName){
    serverLockId = -1;
    name = new char[strlen(myName)];
    sprintf(name,"%s",myName);
    cvWaitQueue = new List();
    
}

ServerCondition::~ServerCondition(){
    delete [] name;
    name = NULL;
    delete cvWaitQueue;
    cvWaitQueue = NULL;
    
    
}


bool ServerCondition::isHeldByMe(int netAddress, int mailBoxNumber){
    if (owner.netAddr == netAddress && owner.mailBoxNum == mailBoxNumber) {
        return true;
    }
    return false;
}

bool pingClient(int clientMachineId){
    DEBUG('s', "In Client Pinging function\n");
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data;
    bool success;
    if (clientMachineId == 0) {
        
    }
    
    try
    {
        data = new char[5];
    }
    catch (bad_alloc& ba)
    {
        cout << "bad_alloc caught: " << ba.what() << endl;
    }
    sprintf(data,"0");
    data[4] = '\0';
    outMailHdr.to = 0;
    outPktHdr.to = clientMachineId;
    outMailHdr.length = (strlen(data)+1);
    success = postOffice->Send(outPktHdr, outMailHdr, data);
    delete [] data;
    data = NULL;
    return success;
    
}


void pingThread(){
    while (true) {
        if (serverLockTable[0].lock != NULL) {
            if (serverLockTable[0].lock->getNet() > 0) {
                if (!pingClient(serverLockTable[0].lock->getNet())) {
                    
                    /*for (int i = 1; i < 6; i++) {
                        if(pingClient(i)){
                            printf("Client ping failed. Giving lock to client with machine id %d\n", i);
                            serverReleaseLock3(0);
                            break;
                        }
                    }*/
                    
                    printf("Client ping failed. Giving lock to client with machine\n");
                    serverReleaseLock3(0);

                    
                    
                }else{
                    DEBUG('p',"Ping Successful In pingThread()\n");
                }
            }else{
                DEBUG('p', "No One to ping\n");
            }
            
        }
        for (int i = 0; i < 20; i++) {
            currentThread->Yield();
        }
    }
    
}


int64_t doTime(int value){
    int64_t a, b, c, d;
    
    timeval t;
    gettimeofday(&t, NULL);
    a = t.tv_sec;
    b = a * 1000000;
    c = b + t.tv_usec;
    cout << "Timestamp " << c << endl;
    d = c * 10;
    d += value;
    cout << "Timestamp w/machineid " << d << endl;
    return d;
    
}

void doReceiveForward(int numServers){
    DEBUG('s',"In initial server function\n");
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    int WORD_LENGTH = 70;
    bool success;
    int rv, sysType, id[7];
    char *data,*strRv;
    char *buffer,*buffer2, *temp, *pch; //
    //char buffer[WORD_LENGTH];
    int lockId, conditionId;
    int mvIndex, mvValue, arrIndex, arraySize;
    int myMachineId, mailBoxNum;
    int64_t time;
    SendStructure *receiveClientMsg;
    myMachineId = postOffice->getMachineId();
    
		try
		{
			data = new char[10];
			buffer = new char[WORD_LENGTH];
		}
		catch (bad_alloc& ba)
		{
			cout << "bad_alloc caught: " << ba.what() << endl;
		}

    while (true) {
        /* Receive the Message */
		
        
        
        //buffer2 = new char[3*(WORD_LENGTH+30)];
        
        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
        DEBUG('s', "Server has received client message in doReceiveForward\n");
        receiveClientMsg = new SendStructure;
        receiveClientMsg = (SendStructure*)buffer;
        
        outMailHdr.to = 1;
        outMailHdr.from = 2;
        time = doTime(myMachineId);
        
        outPktHdr.from = myMachineId;
        
        //something new i was trying
        //        receiveClientMsg.time = time;
        //        receiveClientMsg.serverMachineId = myMachineId;
        receiveClientMsg->time = time;
        receiveClientMsg->serverMachineId = myMachineId;
        
        outMailHdr.length = sizeof(SendStructure);



		outPktHdr.to = myMachineId;
            char *p = (char*)receiveClientMsg;

			  success = postOffice->Send(outPktHdr, outMailHdr, (char*)receiveClientMsg);
            //success = postOffice->Send(outPktHdr, outMailHdr, buffer2);
            if ( !success ) {
                printf("The postOffice Send to server %d failed. You must not have the other Nachos running. Terminating Nachos.\n",myMachineId);
                interrupt->Halt();
            }
            
        for(int i = 0; i < numServers; i++){
			if(i == myMachineId){
				continue;
			}
            outPktHdr.to = i;
            //char *p = (char*)receiveClientMsg;
            DEBUG('s', "Printing byte contents of client msg\n");
            for (int f = 0; f < sizeof(SendStructure); f++) {
                DEBUG('B',"%02x\n", p[f]);
            }
            DEBUG('s', "Sending msg \"%s\" to server %d\n", receiveClientMsg->buf, i);
            
            //DEBUG('s', "Sending msg \"%s\" to server %d\n", receiveClientMsg.buf, i);
            success = postOffice->Send(outPktHdr, outMailHdr, (char*)receiveClientMsg);
            //success = postOffice->Send(outPktHdr, outMailHdr, buffer2);
            if ( !success ) {
                printf("The postOffice Send to server %d failed. You must not have the other Nachos running. Terminating Nachos.\n",i);
                interrupt->Halt();
            }
        }
        //        delete []buffer;
        //        buffer = NULL;
        //        delete []buffer2;
        //        buffer2 = NULL;
        
    }
}


int linear_search(int64_t A[], int imax){
    int temp = 0;
    for (int i = 0; i < imax-1; i++) {
        if (A[i] <= A[i+1]) {
            if (A[i] > -1) {
                temp = i;
            }else{
                temp = i+1;
            }
            
        }else if (A[i] > A[i+1]){
            if(A[i+1] > -1){
                temp = i+1;
            }else{
                temp = i;
            }
            
        }
        
    }
    return temp;
}

int doProcess(SendStructure *processMessage){
    int rv, sysType;
    /* MV values*/
    int arraySize, myMachineId, mailBoxNum, mvId;
    int arrIndex, mvValue;
    char *temp;
    int WORD_LENGTH = 70;
    
    /* Lock and condition values*/
    int lockId, conditionId;
    
    if (processMessage == NULL) {
        printf("Error in doProcess (server.cc). Process Message NULL.\n");
    }
    
    
    temp = new char[WORD_LENGTH];
    sysType = processMessage->syscall;
    lockId = processMessage->lockId;
    conditionId = processMessage->conditionId;
    mvId = processMessage->mvId;
    arraySize = processMessage->arraySize;
    arrIndex = processMessage->arrIndex;
    mvValue = processMessage->mvValue;
    myMachineId = processMessage->clientMachineId;
    mailBoxNum = processMessage->clientMailBox;
    sprintf(temp, "%s", processMessage->buf);
	int forwardServerId = processMessage->serverMachineId;
    
    
    switch (sysType) {
        case CREATE_LOCK://0
            rv = serverCreateLock(temp, myMachineId, mailBoxNum);
            
            break;
        case CREATE_CONDITION://1
            rv = serverCreateCondition(temp, myMachineId, mailBoxNum);
            break;
        case DESTROY_LOCK://2
            rv = serverDestroyLock(lockId);
            break;
        case DESTROY_CONDITION://3
            rv = serverDestroyCondition(conditionId);
            break;
        case ACQUIRE://4
            rv = serverAcquireLock2(lockId, myMachineId, mailBoxNum);
            break;
        case RELEASE://5
            rv = serverReleaseLock2(lockId, forwardServerId);
            break;
        case WAIT://6
            rv = serverWait2(lockId, conditionId, myMachineId, mailBoxNum, forwardServerId);
            DEBUG('l', "After Server Wait in Switch Statement\n");
            break;
        case SIGNAL://7
            rv = serverSignal2(lockId, conditionId);
            break;
        case BROADCAST://8
            rv = serverBroadcast(lockId, conditionId);
            //data = strRv;
            break;
        case CREATE_MV:
            rv = serverCreateMv(temp, arraySize, myMachineId, mailBoxNum);
            break;
        case DESTROY_MV:
            rv = serverDestroyMv(mvId);
            break;
        case GET_MV:
            rv = serverGetMv(mvId, arrIndex);
            break;
        case SET_MV:
            rv = serverSetMv(mvId, arrIndex, mvValue);
            
            break;
            
            
    }
    
    return rv;
}


void TotalOrder(int numServers){
    DEBUG('s',"In initial server function\n");
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    int WORD_LENGTH = 70;
    bool success;
    int rv, sysType, id[7];
    char *data, *strRv;
    char *buffer, *buffer2, *buffer3, *temp, *pch;
    int lockId, conditionId;
    int mvIndex, mvValue, arrIndex, arraySize;
    int myMachineId, mailBoxNum;
    SendStructure *receiveServerMsg, *pendingServerMsg, *timeStampMsg;
    myMachineId = postOffice->getMachineId();
    int64_t recTime, pendTime;
    int forwardServerId, ltrSmallIndex, forwardServerId2;
    List *pendingRequest = new List();
    //int64_t *ltrTable = new int64_t[numServers];
    int64_t ltrTable[numServers];
	int count=0;
    int64_t smallestTimeStamp, remKey, time;
    for (int i = 0; i < numServers; i++) {
        ltrTable[i] = -1;
    }
    
    while (true) {
        /* Receive the Message */
		try
		{
			data = new char[10];
			buffer2 = new char[128];
		}
		catch (bad_alloc& ba)
		{
			cout << "bad_alloc caught: " << ba.what() << endl;
		}
        
        receiveServerMsg = new SendStructure;
        timeStampMsg = new SendStructure;
        
        postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer2);
        DEBUG('s', "Server (machine id = %d) has received message\n", myMachineId);
        
        receiveServerMsg = (SendStructure *)buffer2;
        
        if (receiveServerMsg->syscall == TIME_STAMP) {
            DEBUG('s', "This is a time stamp message\n");
            /* Extract the timestamp and the machine id */
            recTime = receiveServerMsg->time;
            forwardServerId = receiveServerMsg->serverMachineId;
            
            /* Update last time stamp received table */
            ltrTable[forwardServerId] = recTime;
            
            ltrSmallIndex = linear_search(ltrTable, numServers);
            smallestTimeStamp = ltrTable[ltrSmallIndex];
            
        }else if (receiveServerMsg->syscall != TIME_STAMP) {
            
            //if (receiveServerMsg->syscall != TIME_STAMP) {
                time = doTime(myMachineId);
                //time = -1;
//                ltrTable[myMachineId] = time;
                timeStampMsg->syscall = TIME_STAMP;
                timeStampMsg->time = time;
                timeStampMsg->serverMachineId = myMachineId;
                outMailHdr.length = sizeof(SendStructure);
                outMailHdr.to = 1;
                for (int i = 0; i < numServers; i++) {
                    outPktHdr.to = i;
                    //DEBUG('s', "Sending msg \"%s\" to server %d\n", buffer2, i);
//                    if (i != myMachineId) {
                        DEBUG('s', "Sending timestamp %llu message from server %d to server %d\n",time, myMachineId, i);
                        
                        success = postOffice->Send(outPktHdr, outMailHdr, (char*)timeStampMsg);
                        //success = postOffice->Send(outPktHdr, outMailHdr, buffer2);
                        if ( !success ) {
                            printf("The postOffice Send to server %d failed. You must not have the other Nachos running. Terminating Nachos.\n",i);
                            interrupt->Halt();
                        }   
//                    }
                }
                
            //}
            
            DEBUG('s',"The message from the client is: %s\n", receiveServerMsg->buf);
            DEBUG('s',"The timestamp from the other server %llu\n", receiveServerMsg->time, time);
            /* Extract the timestamp and the machine id */
            recTime = receiveServerMsg->time;
            forwardServerId = receiveServerMsg->serverMachineId;
            
            /* Put the request in pending queue in sorted order */
            pendingRequest->SortedInsert((void*)receiveServerMsg, recTime);
			count++;   
            DEBUG('s', "1. Inserting %d system call to the pending request queue and count is %d-------------------\n",receiveServerMsg->syscall,count );
			/* Update last time stamp received table */
            ltrTable[forwardServerId] = recTime;
            	
			/* Scan the LTR table & extract the smallest timestamp */
            //ltrSmallIndex = binary_search(ltrTable, recTime, 0, numServers);
            ltrSmallIndex = linear_search(ltrTable, numServers);
            smallestTimeStamp = ltrTable[ltrSmallIndex];
        
		
		}
        
        
        /* Retrieve first message from pending message queue*/
        pendingServerMsg = (SendStructure*)pendingRequest->SortedRemove(&remKey);
       	
		if (pendingServerMsg == NULL) {
            printf("No messages to process\n");
            continue;
        }
        count--;   
        DEBUG('s', "2. Removing %d system call to the pending request queue and count is %d-------------------\n",pendingServerMsg->syscall,count );
		
		pendTime = pendingServerMsg->time;
        sysType = pendingServerMsg->syscall;
        forwardServerId2 = pendingServerMsg->serverMachineId;
        
        //outPktHdr.to = inPktHdr.from;
        //outMailHdr.to = inMailHdr.from;
        //outMailHdr.from = 1;
        //outPktHdr.from = postOffice->getMachineId();
        
        DEBUG('s', "The pending timestamp is %llu\n", pendTime);
        DEBUG('s', "The smallest timestamp is %llu\n", smallestTimeStamp);
        while(pendTime <= smallestTimeStamp){
            
            rv = doProcess(pendingServerMsg);
            sprintf(data, "%d",rv);
            DEBUG('s', "Smallest time stamp index %d\n", ltrSmallIndex);
            if (forwardServerId2 == myMachineId) {
                outMailHdr.to = pendingServerMsg->clientMailBox;
                outPktHdr.to = pendingServerMsg->clientMachineId;
                
                DEBUG('s', "DONE: send/dont send message to client.\n");
                outMailHdr.length = strlen(data)+1;
                DEBUG('s', "mail header string length %d\n", outMailHdr.length);
                DEBUG('s', "message sent on server side (before NULL) to client \"%s\"\n", data);
                data[9] = '\0';
                DEBUG('l', "message sent on server side to client \"%s\"\n", data);
                //if (sysType != TIME_STAMP) 
				{
                    if (sysType != WAIT) {
                        if (sysType != ACQUIRE) {
                            DEBUG('l', "Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                            success = postOffice->Send(outPktHdr, outMailHdr, data);
                            if ( !success ) {
                                printf("1. The postOffice Send to client %d failed for %d system call. You must not have the other Nachos running. Terminating Nachos.\n", outPktHdr.to,sysType);
                                interrupt->Halt();
                            }
							else if ( success ){
								 printf("The postOffice Send to client %d Successful for %d system call.\n", outPktHdr.to,sysType);
							}
                        }else{
                            if (rv != 1) {
                                DEBUG('l', "Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                                success = postOffice->Send(outPktHdr, outMailHdr, data);
                                if ( !success ) {
                                    printf("2. The postOffice Send to client %d failed for %d system call. You must not have the other Nachos running. Terminating Nachos.\n", outPktHdr.to,sysType);
                                    interrupt->Halt();
                                }
								else if ( success ){
								 printf("The postOffice Send to client %d Successful for %d system call.\n", outPktHdr.to,sysType);
								}
                            }
                            
                            
                        }
                        
                    }else{ //is WAIT
                        if (rv == -1) {
                            DEBUG('l', "Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                            success = postOffice->Send(outPktHdr, outMailHdr, data);
                            if ( !success ) {
                                printf("3. The postOffice Send to client %d failed for %d system call. You must not have the other Nachos running. Terminating Nachos.\n", outPktHdr.to,sysType);
                                interrupt->Halt();
                            }
							else if ( success ){
								 printf("The postOffice Send to client %d Successful for %d system call.\n", outPktHdr.to,sysType);
							}
                        }
                    }
                }
                
            }
            pendingServerMsg = (SendStructure*)pendingRequest->SortedRemove(&remKey);
            if (pendingServerMsg != NULL) {
                pendTime = pendingServerMsg->time;
                forwardServerId = pendingServerMsg->serverMachineId;
                sysType = pendingServerMsg->syscall;
				count--;   
				DEBUG('s', "3. Removing %d system call to the pending request queue and count is %d-------------------\n",pendingServerMsg->syscall,count );
		
            }else{
                break;
            }
        }
        if (pendingServerMsg != NULL) {
            DEBUG('s', "Inserting the last removed message: %s\n", pendingServerMsg->buf);
            pendingRequest->SortedInsert((void*)pendingServerMsg, remKey);
            DEBUG('s', "Inserted the last removed message\n");
			count++;   
			DEBUG('s', "4. Inserting %d system call to the pending request queue and count is %d-------------------\n",pendingServerMsg->syscall,count );
		
        }
        
        
        
        //        delete []buffer;
        //        buffer = NULL;
        //        delete []buffer2;
        //        buffer2 = NULL;
        //        delete []data;
        //        data = NULL;
        //        delete []temp;
        //        temp = NULL;
        //        delete receiveServerMsg;
        //        receiveServerMsg = NULL;
        //        //delete timeStampMsg;
        //        timeStampMsg = NULL;
    }
}

void MultipleServer(int numServers){
    Thread *t;
    t = new Thread("recClienttForServer");
    t->Fork((VoidFunctionPtr)doReceiveForward, numServers);
    
    t = new Thread("TotalOrder");
    t->Fork((VoidFunctionPtr)TotalOrder, numServers);
    
}

void Server(){
    DEBUG('s',"In initial server function\n");
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    int WORD_LENGTH = 70;
    bool success;
    int rv, sysType, id[7];
    char *data, *strRv;
    char *buffer, *buffer2, *temp, *pch;
    int lockId, conditionId;
    int mvIndex, mvValue, arrIndex, arraySize;
    int myMachineId, mailBoxNum;
      
    
    
    while (true) {
        /* Receive the Message */
	data = NULL; buffer = NULL; buffer2 = NULL;
        data = new char[10];
        buffer = new char[WORD_LENGTH];
        buffer2 = new char[WORD_LENGTH];
        
	// buffer = "";
        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
        DEBUG('l', "Server has received message %s of inMailHdr %d\n",buffer,inMailHdr.length);
	// APR15V1
	// The buffer is truncated to inMailHdr.length as the copied data
	// has junk strings in extra char position as noticeable in previous debug output
	buffer[inMailHdr.length] = '\0';
	sprintf(buffer2, "%s\0", buffer);
	DEBUG('l', "Server has received message %s of length truncated to %d \n",buffer2,strlen(buffer2));
        outPktHdr.to = inPktHdr.from;
        outMailHdr.to = inMailHdr.from;
        outMailHdr.from = 1;
        outPktHdr.from = postOffice->getMachineId();
        
        
        
        
        /* Parse the Message */
        sysType = atoi(buffer);
        temp = new char[WORD_LENGTH];
        pch = strtok(buffer2, " ");
        if (TRUE){//sysType == WAIT || sysType == BROADCAST || sysType == SIGNAL) {
            for (int i = 0; i < 7; i++) {
                id[i] = atoi(buffer + 2*i);
                DEBUG('l', "id[%d] = %d\n", i, id[i]);
                if (sysType == GET_MV || sysType == SET_MV || sysType == CREATE_MV
                    || sysType == DESTROY_MV) {
                    id[i] = atoi(pch);
                    pch = strtok(NULL, " ");
                    DEBUG('l',"strtok id[%d] = %d\n", i, id[i]);
                }
                
                
            }
        }else{
            for (int i = 0; i < 4; i++) {
                id[i] = atoi(buffer + 2*i);
                DEBUG('l', "id[%d] = %d\n", i, id[i]);
            }
            id[4] = -1;
        }
        
        pch = strtok(buffer, " ");
        
        while (pch != NULL) {
            
            sprintf(temp, "%s", pch);
            DEBUG('l',"%d\n", atoi(temp));
            DEBUG('l', "%d = pch strlen\n", strlen(pch));
            temp[strlen(pch)] = '\0';
            pch = strtok(NULL, " ");
            
        }
        arraySize = id[1];
        myMachineId = id[2];
        mailBoxNum = id[3];
        mvIndex = id[4];
        arrIndex = id[5];
        mvValue = id[6];
        
        
        /* Validate the Message */
        if (temp == NULL) {
            printf("Null string in server.cc:server()\n");
        }
        
        /* Process the Message */
        switch (sysType) {
            case CREATE_LOCK://0
	        DEBUG('l',"Calling Server create lock for %s\n",temp);
                rv = serverCreateLock(temp, id[2], id[3]);
                sprintf(data,"%d", rv);
                break;
            case CREATE_CONDITION://1
		DEBUG('l',"Calling Server create condition for %s\n",temp);
                rv = serverCreateCondition(temp,id[2], id[3]);
                sprintf(data,"%d",rv);
                break;
            case DESTROY_LOCK://2
                rv = serverDestroyLock(id[1]);
                sprintf(data,"%d",rv);
                break;
            case DESTROY_CONDITION://3
                rv = serverDestroyCondition(id[4]);
                sprintf(data,"%d",rv);
                break;
            case ACQUIRE://4
                rv = serverAcquireLock(id[1], id);
                sprintf(data,"%d",rv);
                break;
            case RELEASE://5
                rv = serverReleaseLock(id[1]);
                sprintf(data,"%d",rv);
                break;
            case WAIT://6
                rv = serverWait(id[1], id[4], id);
                sprintf(data,"%d",rv);
                DEBUG('l', "After Server Wait in Switch Statement\n");
                break;
            case SIGNAL://7
                rv = serverSignal(id[1], id[4], id);
                sprintf(data,"%d",rv);
                break;
            case BROADCAST://8
                rv = serverBroadcast(id[1], id[4]);
                sprintf(data,"%d",rv);
                //data = strRv;
                break;
            case CREATE_MV:
	        DEBUG('l',"Calling Server create MV for %s\n",temp);
                rv = serverCreateMv(temp, arraySize, machineId, mailBoxNum);
                sprintf(data, "%d",rv);
                break;
            case DESTROY_MV:
                rv = serverDestroyMv(mvIndex);
                sprintf(data, "%d",rv);
                break;
            case GET_MV:
                rv = serverGetMv(mvIndex, arrIndex);
                sprintf(data, "%d",rv);
                break;
            case SET_MV:
                rv = serverSetMv(mvIndex, arrIndex, mvValue);
                sprintf(data, "%d",rv);
                break;
            
            
        }
        outMailHdr.length = strlen(data)+1;
        DEBUG('l', "mail header string length %d\n", outMailHdr.length);
        DEBUG('l', "message sent on server side (before NULL) to client \"%s\"\n", data);
        data[9] = '\0';
        DEBUG('l', "message sent on server side to client \"%s\"\n", data);
        
        if (sysType != WAIT) {
            if (sysType != ACQUIRE) {
                DEBUG('l', "Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                success = postOffice->Send(outPktHdr, outMailHdr, data);
                if ( !success ) {
                    printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
                    interrupt->Halt();
                }
            }else{
                if (rv != 1) {
                    DEBUG('l', "AcquireCall:Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                    success = postOffice->Send(outPktHdr, outMailHdr, data);
                    if ( !success ) {
                        printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
                        interrupt->Halt();
                    }
                }
                

            }
                        
        }else{ //is WAIT
            if (rv == -1) {
                DEBUG('l', "WaitCall: Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                success = postOffice->Send(outPktHdr, outMailHdr, data);
                if ( !success ) {
                    printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
                    interrupt->Halt();
                }
            }
            
            
        }
        
	DEBUG('l',"Deleting temp char buffer %s\n",temp);
        delete []temp;
        temp = NULL;
        pch = NULL;
        delete []buffer;
        buffer = NULL;
	delete []buffer2;
        buffer2 = NULL;
        delete []data;
        data = NULL;
	delete []pch;
        pch = NULL;
        
        sysType = -1;
        for (int i = 0; i < 5; i++) {
            id[i] = -1;
        }
        rv = -1;
        outPktHdr.to = -1;
        outMailHdr.to = -1;
        outMailHdr.from = -1;
        outPktHdr.from = -1;
        
        
    }
    
}

void ServerProcess(){
    DEBUG('s',"In initial server function\n");
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    int WORD_LENGTH = 70;
    bool success;
    int rv, sysType, id[7];
    char *data, *strRv;
    char *buffer, *buffer2, *temp, *pch;
    int lockId, conditionId;
    int mvIndex, mvValue, arrIndex, arraySize;
    int myMachineId, mailBoxNum;
    
    
    
    while (true) {
        /* Receive the Message */
        data = NULL; buffer = NULL; buffer2 = NULL;
        data = new char[10];
        buffer = new char[WORD_LENGTH];
        buffer2 = new char[WORD_LENGTH];
        
        // buffer = "";
        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
        DEBUG('l', "Server has received message %s of inMailHdr %d\n",buffer,inMailHdr.length);
        // APR15V1
        // The buffer is truncated to inMailHdr.length as the copied data
        // has junk strings in extra char position as noticeable in previous debug output
        buffer[inMailHdr.length] = '\0';
        sprintf(buffer2, "%s\0", buffer);
        DEBUG('l', "Server has received message %s of length truncated to %d \n",buffer2,strlen(buffer2));
        outPktHdr.to = inPktHdr.from;
        outMailHdr.to = inMailHdr.from;
        outMailHdr.from = 1;
        outPktHdr.from = postOffice->getMachineId();
        
        
        
        
        /* Parse the Message */
        sysType = atoi(buffer);
        temp = new char[WORD_LENGTH];
        pch = strtok(buffer2, " ");
        if (TRUE){//sysType == WAIT || sysType == BROADCAST || sysType == SIGNAL) {
            for (int i = 0; i < 7; i++) {
                id[i] = atoi(buffer + 2*i);
                DEBUG('l', "id[%d] = %d\n", i, id[i]);
                if (sysType == GET_MV || sysType == SET_MV || sysType == CREATE_MV
                    || sysType == DESTROY_MV) {
                    id[i] = atoi(pch);
                    pch = strtok(NULL, " ");
                    DEBUG('l',"strtok id[%d] = %d\n", i, id[i]);
                }
                
                
            }
        }else{
            for (int i = 0; i < 4; i++) {
                id[i] = atoi(buffer + 2*i);
                DEBUG('l', "id[%d] = %d\n", i, id[i]);
            }
            id[4] = -1;
        }
        
        pch = strtok(buffer, " ");
        
        while (pch != NULL) {
            
            sprintf(temp, "%s", pch);
            DEBUG('l',"%d\n", atoi(temp));
            DEBUG('l', "%d = pch strlen\n", strlen(pch));
            temp[strlen(pch)] = '\0';
            pch = strtok(NULL, " ");
            
        }
        arraySize = id[1];
        myMachineId = id[2];
        mailBoxNum = id[3];
        mvIndex = id[4];
        arrIndex = id[5];
        mvValue = id[6];
        
        
        /* Validate the Message */
        if (temp == NULL) {
            printf("Null string in server.cc:server()\n");
        }
        
        /* Process the Message */
        switch (sysType) {
            case CREATE_LOCK://0
                DEBUG('l',"Calling Server create lock for %s\n",temp);
                rv = serverCreateLock(temp, id[2], id[3]);
                sprintf(data,"%d", rv);
                break;
            case CREATE_CONDITION://1
                DEBUG('l',"Calling Server create condition for %s\n",temp);
                rv = serverCreateCondition(temp,id[2], id[3]);
                sprintf(data,"%d",rv);
                break;
            case DESTROY_LOCK://2
                rv = serverDestroyLock(id[1]);
                sprintf(data,"%d",rv);
                break;
            case DESTROY_CONDITION://3
                rv = serverDestroyCondition(id[4]);
                sprintf(data,"%d",rv);
                break;
            case ACQUIRE://4
                rv = serverAcquireLock(id[1], id);
                sprintf(data,"%d",rv);
                break;
            case RELEASE://5
                rv = serverReleaseLock(id[1]);
                sprintf(data,"%d",rv);
                break;
            case WAIT://6
                rv = serverWait(id[1], id[4], id);
                sprintf(data,"%d",rv);
                DEBUG('l', "After Server Wait in Switch Statement\n");
                break;
            case SIGNAL://7
                rv = serverSignal(id[1], id[4], id);
                sprintf(data,"%d",rv);
                break;
            case BROADCAST://8
                rv = serverBroadcast(id[1], id[4]);
                sprintf(data,"%d",rv);
                //data = strRv;
                break;
            case CREATE_MV:
                DEBUG('l',"Calling Server create MV for %s\n",temp);
                rv = serverCreateMv(temp, arraySize, machineId, mailBoxNum);
                sprintf(data, "%d",rv);
                break;
            case DESTROY_MV:
                rv = serverDestroyMv(mvIndex);
                sprintf(data, "%d",rv);
                break;
            case GET_MV:
                rv = serverGetMv(mvIndex, arrIndex);
                sprintf(data, "%d",rv);
                break;
            case SET_MV:
                rv = serverSetMv(mvIndex, arrIndex, mvValue);
                sprintf(data, "%d",rv);
                break;
                
                
        }
        outMailHdr.length = strlen(data)+1;
        DEBUG('l', "mail header string length %d\n", outMailHdr.length);
        DEBUG('l', "message sent on server side (before NULL) to client \"%s\"\n", data);
        data[9] = '\0';
        DEBUG('l', "message sent on server side to client \"%s\"\n", data);
        
        if (sysType != WAIT) {
            if (sysType != ACQUIRE) {
                DEBUG('l', "Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                success = postOffice->Send(outPktHdr, outMailHdr, data);
                if ( !success ) {
                    printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
                    interrupt->Halt();
                }
            }else{
                if (rv != 1) {
                    DEBUG('l', "AcquireCall:Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                    success = postOffice->Send(outPktHdr, outMailHdr, data);
                    if ( !success ) {
                        printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
                        interrupt->Halt();
                    }
                }
                
                
            }
            
        }else{ //is WAIT
            if (rv == -1) {
                DEBUG('l', "WaitCall: Sending message to machine %d mailbox %d\n",outPktHdr.to,outMailHdr.to);
                success = postOffice->Send(outPktHdr, outMailHdr, data);
                if ( !success ) {
                    printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
                    interrupt->Halt();
                }
            }
            
            
        }
        
        DEBUG('l',"Deleting temp char buffer %s\n",temp);
        delete []temp;
        temp = NULL;
        pch = NULL;
        delete []buffer;
        buffer = NULL;
        delete []buffer2;
        buffer2 = NULL;
        delete []data;
        data = NULL;
        delete []pch;
        pch = NULL;
        
        sysType = -1;
        for (int i = 0; i < 5; i++) {
            id[i] = -1;
        }
        rv = -1;
        outPktHdr.to = -1;
        outMailHdr.to = -1;
        outMailHdr.from = -1;
        outPktHdr.from = -1;
        
        
    }
    
}

void Server3(){
    Thread *t;
    t = new Thread("pingThread");
    t->Fork((VoidFunctionPtr)pingThread,0);  
    
    t = new Thread("serverThread");
    t->Fork((VoidFunctionPtr)ServerProcess,0);
    
}


int serverCreateMv(char *buf1, int arraySize, int netAddr, int mailBoxNum){
    int myNet, myMailBox;
    char *buf = new char[strlen(buf1)+1];
    
    sprintf(buf, "%s", buf1);
    buf[strlen(buf1)] = '\0';
    DEBUG('l',"buffer passed %s\n",buf); fflush(stdout);
    for (int i = 0; i < nextMvLocation; i++) {           
        //we are only interested in checking the address space of
       
        if (serverMvTable[i].name != NULL && serverMvTable[i].isDeleted == FALSE){   
            DEBUG('l',"Monitor variable %d has not been deleted. Now check if the name is in the table already.\n", i);
            if (!strcmp(serverMvTable[i].name, buf)) {
                DEBUG('l',"Monitor variable (index = %d) name %s & requested lock name %s\n",i,serverMvTable[i].name, buf);
                                
                DEBUG('l',"Monitor variable (index = %d) %s already exists.\n", i,buf);                    
                
                delete[] buf;
                buf = NULL;
                
                return i;
                                
            }           
        }
    }
    
    //If we reach here, then the monitor variable does not exist in the table
    
    int *newArray = new int[arraySize]; //changed from lockName
    serverMvTable[nextMvLocation].mvArray = newArray;
    serverMvTable[nextMvLocation].arraySize = arraySize;
    serverMvTable[nextMvLocation].name = new char[strlen(buf)];
    sprintf(serverMvTable[nextMvLocation].name, "%s", buf);
    
    serverMvTable[nextMvLocation].usageCount++;
    serverMvTable[nextMvLocation].isDeleted = FALSE;
    
    int mvIndex = nextMvLocation;
    nextMvLocation++;
    
    // APR14-V1 
    // Initialize all the MV variables with value 0
    for ( int i=0;i<arraySize;i++) {
	serverMvTable[mvIndex].mvArray[i] = 0;
    }

    //delete buf; 
    delete[] buf;
    buf = NULL;
    
    
    DEBUG('l',"Successfully created monitor variable %s of index %d\n", (serverMvTable[mvIndex].name), mvIndex);
    return mvIndex;
}

int serverDestroyMv(int mvId){
    if (mvId < 0 || mvId >= MAX_MVS) {
        DEBUG('l',"The monitor variable id passed must be > 0 and < %i.\n", MAX_MVS);
        return -1;

    }
    
    /* Now check if the id passed is within range. If it's within range, then
     * it's actually been created.
     */
    if (mvId <0 || mvId >= nextMvLocation) {
        DEBUG('l',"This mv has not been created. Deletion unnecessary.\n");
        return -1;
    }
    serverMvTable[mvId].usageCount--;
    if (serverMvTable[mvId].isDeleted == FALSE && 
        serverMvTable[mvId].usageCount == 0) {           
        
        DEBUG('l',"Deleting mv with name %s\n", serverMvTable[mvId].name);
        
        //If we reach here then the lock already exists in our address space
        //This is not allowed so we must exit
        
        serverMvTable[mvId].isDeleted = TRUE;
        delete []serverMvTable[mvId].name;
        serverMvTable[mvId].name = NULL;
        delete []serverMvTable[mvId].mvArray;
        serverMvTable[mvId].mvArray = NULL;
            
       
    }else {//if(serverLockTable[lockId].isDeleted == TRUE){
        DEBUG('l',"Monitor Variable at serverMvTable[%i] is already deleted or it is in use\n", mvId);
        
        
    }
    return 0;

}

int serverGetMv(int mvId, int arrIndex){
    if (mvId < 0 || mvId >= MAX_MVS) {
        DEBUG('l',"The monitor variable id passed must be > 0 and < %i.\n", MAX_MVS);
        return -1;
        
    }
    
    /* Now check if the id passed is within range. If it's within range, then
     * it's actually been created.
     */
//    if (mvId <0 || mvId >= nextMvLocation) {
//        DEBUG('l',"This mv has not been created. Returning -1.\n");
//        return -1;
//    }
    
    if (arrIndex >= serverMvTable[mvId].arraySize) {
        DEBUG('l', "Trying to access an array element outside of monitor variable (index = %d) range.\n", mvId);
        return -1;
    }
    
    return serverMvTable[mvId].mvArray[arrIndex];

}

int serverSetMv(int mvId, int arrIndex, int mvValue){
    if (mvId < 0 || mvId >= MAX_MVS) {
        DEBUG('l',"The monitor variable id passed must be > 0 and < %i.\n", MAX_MVS);
        return -1;
        
    }
    
    /* Now check if the id passed is within range. If it's within range, then
     * it's actually been created.
     */
//    if (mvId <0 || mvId >= nextMvLocation) {
//        DEBUG('l',"This mv has not been created. Returning -1.\n");
//        return -1;
//    }
    
    if (arrIndex >= serverMvTable[mvId].arraySize) {
        DEBUG('l', "Trying to access an array element outside of monitor variable (index = %d) range.\n", mvId);
        return -1;
    }
    
    serverMvTable[mvId].mvArray[arrIndex] = mvValue;

    DEBUG('l', "Server sets the value = %d \n",mvValue);
    
    return 0;
    
}

int serverCreateLock(char *buf1, int netAddr, int mailBoxNum){
    
    if (nextConditionLocation > MAX_CONDITIONS) {
        DEBUG('l', "Maximum amount of locks created\n");
        return -1;
    }

    int myNet, myMailBox;
    char *buf = new char[strlen(buf1)+1];
    
    sprintf(buf, "%s", buf1);
    buf[strlen(buf1)] = '\0';
    DEBUG('l',"buffer passed %s\n",buf); fflush(stdout);
    for (int i = 0; i < nextLockLocation; i++) {           
        //we are only interested in checking the address space of
        if (serverLockTable[i].lock != NULL && serverLockTable[i].isDeleted == FALSE){ //&& serverLockTable[i].isToBeDeleted == TRUE) {   
            //if(i == 21)
                //DEBUG('l',"%s", serverLockTable[i].lock->getName());
            if (!strcmp((serverLockTable[i].lock)->getName(), buf)) {
                DEBUG('l',"serverLockTable[%d]->lock name %s & requested lock name %s\n",i,(serverLockTable[i].lock)->getName(), buf);
                myNet = serverLockTable[i].lock->getNet();
                myMailBox = serverLockTable[i].lock->getBox();
                //if (serverLockTable[i].lock->isHeldByMe(netAddr, mailBoxNum)) {                    
                //If we reach here then the lock already exists in our address space
                //This is not allowed so we must exit
                DEBUG('l',"Lock %s ( index = %d ) already exists.\n",buf,i);                    
                
                delete[] buf;
                buf = NULL;
                
                return i;
                //}                
            }           
        }
    }
    
    //If we reach here, then the lock does not exist in our address space
    
    DEBUG('l',"ServerLock passed argument %s\n",buf1);
    DEBUG('l',"Calling ServerLock with %s\n",buf);
    ServerLock *newLock = new ServerLock(buf); //changed from lockName
    serverLockTable[nextLockLocation].lock = newLock;
    serverLockTable[nextLockLocation].lock->setOwner(netAddr, mailBoxNum);
    serverLockTable[nextLockLocation].isDeleted = FALSE;
    serverLockTable[nextLockLocation].isToBeDeleted = TRUE; // FEB27V2 //
    
    int lockIndex = nextLockLocation;
    nextLockLocation++;
    
    //delete buf; 
    delete[] buf;
    buf = NULL;
    
    
    DEBUG('l',"Successfully created lock %s of index %d\n", (serverLockTable[lockIndex].lock)->getName(), lockIndex);
    return lockIndex;
    
}


int serverCreateCondition(char *buf1, int netAddr, int mailBoxNum){
    //must acquire a kerneltablelock
    if (nextConditionLocation > MAX_CONDITIONS) {
        DEBUG('l', "Maximum amount of condition variables created\n");
        return -1;
    }
    int myNet, myMailBox;
    char *buf = new char[strlen(buf1)+1];
    sprintf(buf, "%s", buf1);
    buf[strlen(buf1)] = '\0';
    DEBUG('l',"buffer passed createCond %s\n",buf); fflush(stdout);
    for (int i = 0; i < nextConditionLocation; i++) { 
     
        if (serverConditionTable[i].condition != NULL && serverConditionTable[i].isDeleted == FALSE){// && serverConditionTable[i].isToBeDeleted == FALSE) {
            
            if (!strcmp((serverConditionTable[i].condition)->getName(), buf)) {
                DEBUG('l',"Condition name %s and requested condition name %s\n", (serverConditionTable[i].condition)->getName(), buf);
                myNet = serverConditionTable[i].condition->getNet();
                myMailBox = serverConditionTable[i].condition->getBox();
                //if (serverConditionTable[i].condition->isHeldByMe(netAddr, mailBoxNum)) {
                
                //If we reach here then the lock already exists in our address space
                //This is not allowed so we must exit
                DEBUG('l',"Condition %s already exists.\n", buf);
                delete []buf;
                buf = NULL;
                
                
                return i;
                //}
                
            }           
            
        }
    }
    
    //If we reach here, then the lock does not exist in our address space
    
    ServerCondition *newCondition = new ServerCondition(buf); //changed from lockName
    serverConditionTable[nextConditionLocation].condition = newCondition;
    serverConditionTable[nextConditionLocation].condition->setOwner(netAddr, mailBoxNum);
    serverConditionTable[nextConditionLocation].isDeleted = FALSE;
    serverConditionTable[nextConditionLocation].isToBeDeleted = TRUE;
    int conditionIndex = nextConditionLocation;
    nextConditionLocation++;
    
    //delete buf;   
    delete[] buf;
    buf = NULL;
    
    
    DEBUG('l',"Successfully created condition %s of index %d\n", (serverConditionTable[conditionIndex].condition)->getName(), conditionIndex);
    return conditionIndex;
}

int serverDestroyLock(int lockId){
    char *lockName;
    int myNet, myMailBox;
    //Destroy a lock identified by its "lockId"
    DEBUG('l', "destroy lock function\n");
    if (lockId < 0 ||
        lockId >= MAX_LOCKS) {
        DEBUG('l',"The lock id passed must be > 0 and < %i.\n", MAX_LOCKS);
        return -1;
    }
    
        
    /* Now check if the id passed is within range. If it's within range, then
     * it's actually been created.
     */
    if (lockId <0 || lockId >= nextLockLocation) {
        DEBUG('l',"This lock has not been created. Deletion unnecessary.\n");
        return -1;
    }
    
    /*
     * We check if the lock is NULL
     */
    if (serverLockTable[lockId].lock != NULL) {
        if (serverLockTable[lockId].isDeleted == FALSE && (serverLockTable[lockId].isToBeDeleted && serverLockTable[lockId].lock->getStatus() == FREE)) {//serverLockTable[lockId].usageCount == 0){
            //           
            DEBUG('l',"Lock name %s\n", (serverLockTable[lockId].lock)->getName());
            myNet = serverLockTable[lockId].lock->getNet();
            myMailBox = serverLockTable[lockId].lock->getBox();
            if (serverLockTable[lockId].lock->isHeldByMe(myNet, myMailBox)) {
                DEBUG('l',"deleting Lock with name %s\n", (serverLockTable[lockId].lock)->getName());
                
                //If we reach here then the lock already exists in our address space
                //This is not allowed so we must exit
                lockName = (serverLockTable[lockId].lock)->getName();
                serverLockTable[lockId].isDeleted = TRUE;
                DEBUG('l',"lock status %d\n", serverLockTable[lockId].lock->getStatus());
                delete serverLockTable[lockId].lock;
                serverLockTable[lockId].lock = NULL;
                
            }else{
                DEBUG('l',"%s is not the owner of the lock %s\n", currentThread->getName(), (serverLockTable[lockId].lock)->getName());
                
                return -1;            
            }        
        }else {//if(serverLockTable[lockId].isDeleted == TRUE){
            DEBUG('l',"Lock at serverLockTable[%i] is already deleted or it is in use\n", lockId);
            
            return -1;
        }
    }else{ //if(serverLockTable[lockId].lock == NULL){
        DEBUG('l',"Lock at serverLockTable[%i] is Null\n", lockId);
        
        return -1;
    }
    
    DEBUG('l',"Successfully deleted lock %s with id %d\n",lockName, lockId);
    
    return 0;
    
}
int serverDestroyCondition(int conditionId){
    char *conditionName;
    int myNet, myMailBox;
    //Destroy a condition identified by its "conditionId"
    DEBUG('l', "destroy condition function\n");
    if (conditionId < 0 ||
        conditionId >= MAX_CONDITIONS) {
        DEBUG('l',"The condition id passed must be > 0 and < %i.\n", MAX_CONDITIONS);
        return -1;
    }
    
    if (conditionId <0 || conditionId >= nextConditionLocation) {
        DEBUG('l',"This condition has not been created. Deletion unnecessary.\n");
        return -1;
    }
    
    
    /*
     * We check if the condition is NULL
     */
    //serverConditionTable[conditionId].usageCount--;
    if (serverConditionTable[conditionId].condition != NULL) {
        if (serverConditionTable[conditionId].isDeleted == FALSE && 
            serverConditionTable[conditionId].usageCount == 0) {           
            DEBUG('l',"condition name %s\n", (serverConditionTable[conditionId].condition)->getName());
            myNet = serverConditionTable[conditionId].condition->getNet();
            myMailBox = serverConditionTable[conditionId].condition->getBox();
            if (serverConditionTable[conditionId].condition->isHeldByMe(myNet, myMailBox)) {
                DEBUG('l',"deleting condition with name %s\n", (serverConditionTable[conditionId].condition)->getName());
                
                //If we reach here then the condition already exists in our address space
                //This is not allowed so we must exit
                conditionName = (serverConditionTable[conditionId].condition)->getName();
                serverConditionTable[conditionId].isDeleted = TRUE;
                delete serverConditionTable[conditionId].condition;
                serverConditionTable[conditionId].condition = NULL;
                
            }else{
                DEBUG('l',"%s is not the owner of the condition %s\n", currentThread->getName(), (serverConditionTable[conditionId].condition)->getName());
                
                return -1;            
            }        
        }else {//if(serverConditionTable[conditionId].isDeleted == TRUE){
            DEBUG('l',"condition at serverConditionTable[%i] is already deleted or it is in use\n", conditionId);
            
            return -1;
        }
    }else{ //if(serverConditionTable[conditionId].condition == NULL){
        DEBUG('l',"condition at serverConditionTable[%i] is Null\n", conditionId);
        
        return -1;
    }
    
    DEBUG('l',"Successfully deleted condition %s with id %d\n",conditionName, conditionId);
    
    return 0;
}

int serverAcquireLock(int lockId, int id[]){
    int lockNet, lockMailBox;
    
    int myMachineId = id[2];
    int myMailBox = id[3];

    char *msg;
    //Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to acquire a lock of invalid ID %d. Acquire request denied\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to acquire lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        
        return -1;
	}
    
	//Check if the lock belongs to different process
    lockNet = serverLockTable[lockId].lock->getNet();
    lockMailBox = serverLockTable[lockId].lock->getBox();
    
    /*
     * If the lock is free, then I should message the user.
     * Otherwise I should just add a message to the serverLock
     * list for future messaging.
     */
    
    serverLockTable[lockId].usageCount++;
    
    if (serverLockTable[lockId].lock->getStatus() == BUSY) {
        DEBUG('l',"%s is being held by machine %d mailbox/thread %d\n", serverLockTable[lockId].lock->getName(),lockNet, lockMailBox);
        msg = new char[10];
        sprintf(msg,"%d %d %d",lockId, myMachineId, myMailBox);
        msg[9] = '\0';
        serverLockTable[lockId].lock->listAppend(msg);
        serverLockTable[lockId].isToBeDeleted = FALSE;
        return 1;
        
    }
    
    serverLockTable[lockId].lock->setStatus(BUSY);
    serverLockTable[lockId].lock->setNet(myMachineId);
    serverLockTable[lockId].lock->setOwner(myMachineId, myMailBox);
    serverLockTable[lockId].isToBeDeleted = FALSE; // FEB27V2 //
    
    
    DEBUG('l',"Successfully acquired lock %s of index %d\n", (serverLockTable[lockId].lock)->getName(), lockId);

    return 0;
}

int serverAcquireLock2(int lockId, int locMachineId, int mailbox){
    int lockNet, lockMailBox;
    
    int myMachineId = locMachineId;
    int myMailBox = mailbox;

    char *msg;
    //Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to acquire a lock of invalid ID %d. Acquire request denied\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to acquire lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        
        return -1;
	}
    
	//Check if the lock belongs to different process
    lockNet = serverLockTable[lockId].lock->getNet();
    lockMailBox = serverLockTable[lockId].lock->getBox();
    
    /*
     * If the lock is free, then I should message the user.
     * Otherwise I should just add a message to the serverLock
     * list for future messaging.
     */
    
    serverLockTable[lockId].usageCount++;
    
    if (serverLockTable[lockId].lock->getStatus() == BUSY) {
        DEBUG('l',"%s is being held by machine %d mailbox/thread %d\n", serverLockTable[lockId].lock->getName(),lockNet, lockMailBox);
        msg = new char[10];
        sprintf(msg,"%d %d %d",lockId, myMachineId, myMailBox);
        msg[9] = '\0';
        serverLockTable[lockId].lock->listAppend(msg);
        serverLockTable[lockId].isToBeDeleted = FALSE;
        return 1;
        
    }
    
    serverLockTable[lockId].lock->setStatus(BUSY);
    serverLockTable[lockId].lock->setNet(myMachineId);
    serverLockTable[lockId].lock->setOwner(myMachineId, myMailBox);
    serverLockTable[lockId].isToBeDeleted = FALSE; // FEB27V2 //
    
    
    DEBUG('l',"Successfully acquired lock %s of index %d\n", (serverLockTable[lockId].lock)->getName(), lockId);

    return 0;
}

int serverReleaseLock(int lockId){
    int lockNet, lockMailBox;
    //(PacketHeader *)outPktHdr;
    //(MailHeader *)outMailHdr;
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
    
    char *msg, *removedMsg;
    //Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to release a lock of invalid ID %d. Release request denied\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to release lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        
        return -1;
	}
    
	//Check if the lock belongs to different process
    lockNet = serverLockTable[lockId].lock->getNet();
    lockMailBox = serverLockTable[lockId].lock->getBox();
    
    /*
     * If the lock is free, then I should message the user.
     * Otherwise I should just add a message to the serverLock
     * list for future messaging.
     */
    
    /*
          */
    msg = new char[10];
    removedMsg = (char*)serverLockTable[lockId].lock->listRemove();
    serverLockTable[lockId].usageCount--;
    if (removedMsg != NULL) {
       
        sprintf(msg, "%s", removedMsg);
        msg[9] = '\0';
        outPktHdr.to = atoi(msg+2);//ACQUIRE LIST
        outMailHdr.to = atoi(msg+4); //ACQUIRE LIST
        outMailHdr.from = 1;
        outMailHdr.length = 10;
        
        bool success = postOffice->Send(outPktHdr, outMailHdr, msg);
        if ( !success ) {
            printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
            interrupt->Halt();
        }else{
            printf("serverreleaselock: The postOffice Send to machine ID %d and mailbox %d successful.................................\n",outPktHdr.to,outMailHdr.to);
        }
        serverLockTable[lockId].lock->setNet(outPktHdr.to);
        serverLockTable[lockId].lock->setOwner(outPktHdr.to, outMailHdr.to);
        
    }else{
        serverLockTable[lockId].lock->setStatus(FREE);
        serverLockTable[lockId].isToBeDeleted = TRUE;
		DEBUG('s',"No one waiting on the lock.....................................................................");
    }
    delete []msg;
    msg = NULL;
    removedMsg = NULL;
    DEBUG('l',"Successfully released lock %s of index %d\n", (serverLockTable[lockId].lock)->getName(), lockId);

    
    return 0;
}

int serverReleaseLock2(int lockId, int forwardServerId){
    int lockNet, lockMailBox;
    //(PacketHeader *)outPktHdr;
    //(MailHeader *)outMailHdr;
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
	int	myMachineId = postOffice->getMachineId();
    
    char *msg, *removedMsg;
    //Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to release a lock of invalid ID %d. Release request denied\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to release lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        
        return -1;
	}
    
	//Check if the lock belongs to different process
    lockNet = serverLockTable[lockId].lock->getNet();
    lockMailBox = serverLockTable[lockId].lock->getBox();
    
    /*
     * If the lock is free, then I should message the user.
     * Otherwise I should just add a message to the serverLock
     * list for future messaging.
     */
    msg = new char[10];
    removedMsg = (char*)serverLockTable[lockId].lock->listRemove();
    serverLockTable[lockId].usageCount--;
    if (removedMsg != NULL) {
        sprintf(msg, "%s", removedMsg);
        msg[9] = '\0';
        /* Parse the message */
        
        outPktHdr.to = atoi(msg+2);//ACQUIRE LIST
        outMailHdr.to = atoi(msg+4); //ACQUIRE LIST
        outMailHdr.from = 1;
        outMailHdr.length = 10;
        
		if( myMachineId == forwardServerId ){
        bool success = postOffice->Send(outPktHdr, outMailHdr, msg);
		
        if ( !success ) {
            printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
            interrupt->Halt();
        }else{
			printf("serverreleaselock: The postOffice Send to machine ID %d and mailbox %d successful.................................\n",outPktHdr.to,outMailHdr.to);
		}
		}
        serverLockTable[lockId].lock->setNet(outPktHdr.to);
        serverLockTable[lockId].lock->setOwner(outPktHdr.to, outMailHdr.to);
        
    }else{
        serverLockTable[lockId].lock->setStatus(FREE);
        serverLockTable[lockId].isToBeDeleted = TRUE;
		DEBUG('s',"No one waiting on the lock.....................................................................");
    }
    delete []msg;
    msg = NULL;
    removedMsg = NULL;
    DEBUG('l',"Successfully released lock %s of index %d\n", (serverLockTable[lockId].lock)->getName(), lockId);

    
    return 0;
}

int serverReleaseLock3(int lockId){
    int lockNet, lockMailBox;
    //(PacketHeader *)outPktHdr;
    //(MailHeader *)outMailHdr;
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
    
    char *msg, *removedMsg;
    //Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to release a lock of invalid ID %d. Release request denied\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to release lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        // Release Kernel table lock
        
        return -1;
	}
    
	//Check if the lock belongs to different process
    lockNet = serverLockTable[lockId].lock->getNet();
    lockMailBox = serverLockTable[lockId].lock->getBox();
    
    /*
     * If the lock is free, then I should message the user.
     * Otherwise I should just add a message to the serverLock
     * list for future messaging.
     */
    
    /*
     sprintf(msg, "%s", removedMsg);
     msg[9] = '\0';
     outPktHdr.to = atoi(msg+2);//ACQUIRE LIST
     outMailHdr.to = atoi(msg+4); //ACQUIRE LIST
     outMailHdr.from = 1;
     outMailHdr.length = 10;
     
     bool success = postOffice->Send(outPktHdr, outMailHdr, msg);
     if ( !success ) {
     printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
     interrupt->Halt();
     }else{
     printf("serverreleaselock: The postOffice Send to machine ID %d and mailbox %d successful.................................\n",outPktHdr.to,outMailHdr.to);
     }
     serverLockTable[lockId].lock->setNet(outPktHdr.to);
     serverLockTable[lockId].lock->setOwner(outPktHdr.to, outMailHdr.to);
     */
    msg = new char[10];
    removedMsg = (char*)serverLockTable[lockId].lock->listRemove();
    serverLockTable[lockId].usageCount--;
    if (removedMsg != NULL) {
        
        
        while (true) {
            sprintf(msg, "%s", removedMsg);
            msg[9] = '\0';
            /* Parse the message */
            
            outPktHdr.to = atoi(msg+2);//ACQUIRE LIST
            outMailHdr.to = atoi(msg+4); //ACQUIRE LIST
            outMailHdr.from = 1;
            outMailHdr.length = 10;
            
            if(pingClient(outPktHdr.to)){
                DEBUG('p', "Ping Successful\n");
                bool success = postOffice->Send(outPktHdr, outMailHdr, msg);
                if ( !success ) {
                    printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
                    interrupt->Halt();
                }else{
                    DEBUG('l',"serverreleaselock: The postOffice Send to machine ID %d and mailbox %d successful.................................\n",outPktHdr.to,outMailHdr.to);
                }
                serverLockTable[lockId].lock->setNet(outPktHdr.to);
                serverLockTable[lockId].lock->setOwner(outPktHdr.to, outMailHdr.to);
                break;
            }else{
                removedMsg = (char*)serverLockTable[lockId].lock->listRemove();
                if (removedMsg == NULL) {
                    break;
                }
                serverLockTable[lockId].usageCount--;
            }
            
            //serverLockTable[lockId].lock->listPrepend(removedMsg);
            
        }
        
        
        
        
    }else{
        serverLockTable[lockId].lock->setStatus(FREE);
        serverLockTable[lockId].isToBeDeleted = TRUE;
		DEBUG('s',"No one waiting on the lock.....................................................................");
    }
    delete []msg;
    msg = NULL;
    removedMsg = NULL;
    DEBUG('l',"Successfully released lock %s of index %d\n", (serverLockTable[lockId].lock)->getName(), lockId);
    
    
    return 0;
}


int serverWait(int lockId, int conditionId, int id[]){
    char *msg, *removedMsg;
    int conditionNet, conditionMailBox;
    int myLockId;
    int myMachineId, myMailBox;
    
    myMachineId = id[2];
    myMailBox = id[3];
    
    
	//Check	if the conditionId is out of range
	if ( conditionId < 0 || conditionId >= nextConditionLocation  )	{ 
        DEBUG('l',"%s is trying to wait on a Condition of invalid ID %d. Wait request denied \n",currentThread->getName(),conditionId);
        return -1;
	}
    
    
	//Check	if lock	is already deleted
	if (serverConditionTable[conditionId].isDeleted) {
        DEBUG('l',"%s is trying to wait on a condition of ID %d that is already deleted\n",currentThread->getName(),conditionId); 
        return -1;
	}
    
	//Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to wait on a lock of invalid ID %d. Wait request denied\n",currentThread->getName(),lockId);       
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to wait on a lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        return -1;
	}
    //Check if the lock for this specific condition has been set
    if (serverConditionTable[conditionId].condition->getLockId() < 0) {
        DEBUG('l', "Setting lock id of condition %s (index = %d) to %d\n",serverConditionTable[conditionId].condition->getName(),conditionId, lockId);
        serverConditionTable[conditionId].condition->setLockId(lockId);
    }
    
    //Check if the lock id being passed is the correct one to wait for
    if (serverConditionTable[conditionId].condition->getLockId() != lockId) {
        DEBUG('l', "Not allowed to wait on a lock that belongs to a different CV\n");
        return -1;
    }
    
    conditionNet = serverLockTable[lockId].lock->getNet();
    conditionMailBox = serverLockTable[lockId].lock->getBox();
    
	
	//Wait on the lock
	DEBUG('l',"Thread %s is going to wait on a lock of name %s and Id %d for the condition %s  with Id %d\n", currentThread->getName(),serverLockTable[lockId].lock->getName(),lockId,serverConditionTable[conditionId].condition->getName(),conditionId);
    
    msg = new char[10];
    sprintf(msg,"%d %d %d",1, myMachineId, myMailBox);
    msg[9] = '\0';
    DEBUG('l', "Server wait.  Appended \"%s\" to condition %d wait queue.  Wait for reply on machine %d at mailbox %d\n", msg, conditionId, conditionNet, conditionMailBox);
    
    serverConditionTable[conditionId].condition->listAppend(msg);
    serverConditionTable[conditionId].usageCount++;

    int relRetVal = serverReleaseLock(lockId);
    DEBUG('l', "Releasing lock in wait. Return value is %d\n", relRetVal);
    
    //serverConditionTable[conditionId].isToBeDeleted = FALSE;
    //delete [] msg;
    msg = NULL;
    removedMsg = NULL;
    return 0;
}
int serverSignal(int lockId, int conditionId, int id[]){
    char *msg, *removedMsg, *buf, *msg2;
    int conditionNet, conditionMailBox;
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
    int retVal = 0;
    DEBUG('l', "Inside the server signal\n");
	//Check	if the conditionId is out of range
	if ( conditionId < 0 || conditionId >= nextConditionLocation  )	{
        DEBUG('l',"%d is next condition location\n",nextConditionLocation);
        DEBUG('l',"%s is trying to signal on a Condition of invalid ID %d. Signal request denied \n",currentThread->getName(),conditionId);
        //return "-1";
        return -1;
	}
    
    
	//Check	if lock	is already deleted
	if (serverConditionTable[conditionId].isDeleted) {
        DEBUG('l',"%s is trying to signal on a condition of ID %d that is already deleted\n",currentThread->getName(),conditionId); 
        //return "-1";
        return -1;
	}
    
    
	//Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to signal on a lock of invalid ID %d. Signal request denied\n",currentThread->getName(),lockId);       
        //return "-1";
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to signal on a lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        //return "-1";
        return -1;
	}
    
    //Check if the lock id being passed is the correct one to wait for
    if (serverConditionTable[conditionId].condition->getLockId() != lockId) {
        DEBUG('l', "Condition lock Id %d and the lock id passed %d\n", serverConditionTable[conditionId].condition->getLockId(),lockId);
        DEBUG('l', "Not allowed to signal a lock that belongs to a different CV\n");
        //return "-1";
        return -1;
    }
    
    //Examine the latest request message and signal the machine that is 
    //waiting
    msg = new char[10];
    removedMsg = (char*)serverConditionTable[conditionId].condition->listRemove();
    //DEBUG('l', "removed Message in signal %s\n", removedMsg);
    serverConditionTable[conditionId].usageCount--;
    if (removedMsg != NULL) {
        sprintf(msg, "%s", removedMsg);
        msg[9] = '\0';
        /* Parse the message */
        
        outPktHdr.to = atoi(msg+2);//ACQUIRE LIST
        outMailHdr.to = atoi(msg+4); //ACQUIRE LIST
        outMailHdr.from = 1;
        outMailHdr.length = 10;
        DEBUG('l',"Server Signal. Mail to machine %d at mailbox/thread %d.\n",outPktHdr.to, outMailHdr.to);
        
        
        //serverLockTable[lockId].lock->setNet(outPktHdr.to);
        //serverConditionTable[conditionId].condition->setNet(outPktHdr.to);
        
        //serverLockTable[lockId].lock->setOwner(outPktHdr.to,outMailHdr.to);
        retVal = 1;
        //removedMsg = (char*)serverLockTable[lockId].lock->listRemove();
        int id2[4];
        id2[2] = outPktHdr.to;
        id2[3] = outMailHdr.to;
        int acqRetVal = serverAcquireLock(lockId, id2);
        DEBUG('l', "Acquiring lock in signal. Return value is %d\n", acqRetVal);
        

        return 1;
        
    }else{
        DEBUG('l', "No one is waiting on lock %s (index = %d)\n",serverLockTable[lockId].lock->getName(),lockId); 
    }
    
    //Now check if anyone is waiting on this lock at all
    if (serverConditionTable[conditionId].condition->isEmpty()) {
        DEBUG('l', "Wait queue for condition is empty. Set lock id to -1.\n");
        serverConditionTable[conditionId].condition->setLockId(-1);
    }
    //delete []msg;
    msg = NULL;
    removedMsg = NULL;
    
    
    //return "0"; // 1 for signalling waiter, 0 for not signalling anyone
    return 0;
}

int serverWait2(int lockId, int conditionId, int locMachineId, int mailBoxNum,int forwardServerId){
    char *msg, *removedMsg;
    int conditionNet, conditionMailBox;
    int myLockId;
    int myMachineId, myMailBox;
    
    myMachineId = locMachineId;
    myMailBox = mailBoxNum;
    
    
	//Check	if the conditionId is out of range
	if ( conditionId < 0 || conditionId >= nextConditionLocation  )	{ 
        DEBUG('l',"%s is trying to wait on a Condition of invalid ID %d. Wait request denied \n",currentThread->getName(),conditionId);
        return -1;
	}
    
    
	//Check	if lock	is already deleted
	if (serverConditionTable[conditionId].isDeleted) {
        DEBUG('l',"%s is trying to wait on a condition of ID %d that is already deleted\n",currentThread->getName(),conditionId); 
        return -1;
	}
    
	//Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to wait on a lock of invalid ID %d. Wait request denied\n",currentThread->getName(),lockId);       
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to wait on a lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        return -1;
	}
    //Check if the lock for this specific condition has been set
    if (serverConditionTable[conditionId].condition->getLockId() < 0) {
        DEBUG('l', "Setting lock id of condition %s (index = %d) to %d\n",serverConditionTable[conditionId].condition->getName(),conditionId, lockId);
        serverConditionTable[conditionId].condition->setLockId(lockId);
    }
    
    //Check if the lock id being passed is the correct one to wait for
    if (serverConditionTable[conditionId].condition->getLockId() != lockId) {
        DEBUG('l', "Not allowed to wait on a lock that belongs to a different CV\n");
        return -1;
    }
    
    conditionNet = serverLockTable[lockId].lock->getNet();
    conditionMailBox = serverLockTable[lockId].lock->getBox();
    
	
	//Wait on the lock
	DEBUG('l',"Thread %s is going to wait on a lock of name %s and Id %d for the condition %s  with Id %d\n", currentThread->getName(),serverLockTable[lockId].lock->getName(),lockId,serverConditionTable[conditionId].condition->getName(),conditionId);
    
    msg = new char[10];
    sprintf(msg,"%d %d %d",1, myMachineId, myMailBox);
    msg[9] = '\0';
    DEBUG('l', "Server wait.  Appended \"%s\" to condition %d wait queue.  Wait for reply on machine %d at mailbox %d\n", msg, conditionId, conditionNet, conditionMailBox);
    
    serverConditionTable[conditionId].condition->listAppend(msg);
    serverConditionTable[conditionId].usageCount++;
    
    int relRetVal = serverReleaseLock2(lockId, forwardServerId);
    DEBUG('l', "Releasing lock in wait. Return value is %d\n", relRetVal);
    
    //serverConditionTable[conditionId].isToBeDeleted = FALSE;
    //delete [] msg;
    msg = NULL;
    removedMsg = NULL;
    return 0;
}
int serverSignal2(int lockId, int conditionId){
    char *msg, *removedMsg, *buf, *msg2;
    int conditionNet, conditionMailBox;
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
    int retVal = 0;
    DEBUG('l', "Inside the server signal\n");
	//Check	if the conditionId is out of range
	if ( conditionId < 0 || conditionId >= nextConditionLocation  )	{
        DEBUG('l',"%d is next condition location\n",nextConditionLocation);
        DEBUG('l',"%s is trying to signal on a Condition of invalid ID %d. Signal request denied \n",currentThread->getName(),conditionId);
        //return "-1";
        return -1;
	}
    
    
	//Check	if lock	is already deleted
	if (serverConditionTable[conditionId].isDeleted) {
        DEBUG('l',"%s is trying to signal on a condition of ID %d that is already deleted\n",currentThread->getName(),conditionId); 
        //return "-1";
        return -1;
	}
    
    
	//Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to signal on a lock of invalid ID %d. Signal request denied\n",currentThread->getName(),lockId);       
        //return "-1";
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to signal on a lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        //return "-1";
        return -1;
	}
    
    //Check if the lock id being passed is the correct one to wait for
    if (serverConditionTable[conditionId].condition->getLockId() != lockId) {
        DEBUG('l', "Condition lock Id %d and the lock id passed %d\n", serverConditionTable[conditionId].condition->getLockId(),lockId);
        DEBUG('l', "Not allowed to signal a lock that belongs to a different CV\n");
        //return "-1";
        return -1;
    }
    
    //Examine the latest request message and signal the machine that is 
    //waiting
    msg = new char[10];
    removedMsg = (char*)serverConditionTable[conditionId].condition->listRemove();
    //DEBUG('l', "removed Message in signal %s\n", removedMsg);
    serverConditionTable[conditionId].usageCount--;
    if (removedMsg != NULL) {
        sprintf(msg, "%s", removedMsg);
        msg[9] = '\0';
        /* Parse the message */
        
        outPktHdr.to = atoi(msg+2);//ACQUIRE LIST
        outMailHdr.to = atoi(msg+4); //ACQUIRE LIST
        outMailHdr.from = 1;
        outMailHdr.length = 10;
        DEBUG('l',"Server Signal. Mail to machine %d at mailbox/thread %d.\n",outPktHdr.to, outMailHdr.to);
        
        
        //serverLockTable[lockId].lock->setNet(outPktHdr.to);
        //serverConditionTable[conditionId].condition->setNet(outPktHdr.to);
        
        //serverLockTable[lockId].lock->setOwner(outPktHdr.to,outMailHdr.to);
        retVal = 1;
        //removedMsg = (char*)serverLockTable[lockId].lock->listRemove();
        int id2[4];
        id2[2] = outPktHdr.to;
        id2[3] = outMailHdr.to;
        int acqRetVal = serverAcquireLock2(lockId, outPktHdr.to, outMailHdr.to);
        DEBUG('l', "Acquiring lock in signal. Return value is %d\n", acqRetVal);
               
        return 1;
        
    }else{
        DEBUG('l', "No one is waiting on lock %s (index = %d)\n",serverLockTable[lockId].lock->getName(),lockId); 
    }
    
    //Now check if anyone is waiting on this lock at all
    if (serverConditionTable[conditionId].condition->isEmpty()) {
        DEBUG('l', "Wait queue for condition is empty. Set lock id to -1.\n");
        serverConditionTable[conditionId].condition->setLockId(-1);
    }
    //delete []msg;
    msg = NULL;
    removedMsg = NULL;
    
    
    //return "0"; // 1 for signalling waiter, 0 for not signalling anyone
    return 0;
}


int serverBroadcast(int lockId, int conditionId){
    int id[7];
    if ( conditionId < 0 || conditionId >= nextConditionLocation  )	{
        DEBUG('l',"%d is next condition location\n",nextConditionLocation);
        DEBUG('l',"%s is trying to signal on a Condition of invalid ID %d. Signal request denied \n",currentThread->getName(),conditionId);
        //return "-1";
        return -1;
	}
    
    
	//Check	if lock	is already deleted
	if (serverConditionTable[conditionId].isDeleted) {
        DEBUG('l',"%s is trying to signal on a condition of ID %d that is already deleted\n",currentThread->getName(),conditionId); 
        //return "-1";
        return -1;
	}
    
    
	//Check	if the lockId is out of	range
	if ( lockId < 0	|| lockId >= nextLockLocation  )	{ 
        DEBUG('l',"%s is trying to signal on a lock of invalid ID %d. Signal request denied\n",currentThread->getName(),lockId);       
        //return "-1";
        return -1;
	}
    
	//Check	if lock	is already deleted
	if (serverLockTable[lockId].isDeleted) {
        DEBUG('l',"%s is trying to signal on a lock of ID %d that is already deleted\n",currentThread->getName(),lockId);
        //return "-1";
        return -1;
	}
    
    //Check if the lock id being passed is the correct one to wait for
    if (serverConditionTable[conditionId].condition->getLockId() != lockId) {
        DEBUG('l', "Condition lock Id %d and the lock id passed %d\n", serverConditionTable[conditionId].condition->getLockId(),lockId);
        DEBUG('l', "Not allowed to signal a lock that belongs to a different CV\n");
        //return "-1";
        return -1;
    }

    while (TRUE) {
        serverSignal(lockId, conditionId, id);
        if (serverConditionTable[conditionId].condition->isEmpty()) {
            DEBUG('l',"Broadcast finished. Wait queue for condition is empty.\n"); 
            break;
        }
    }
    
    return 0;
}


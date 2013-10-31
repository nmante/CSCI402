//
//  server.h
//  
//
//  Created by Nii Mante on 3/14/12.
//  Copyright (c) 2012 University of Southern California. All rights reserved.
//

#ifndef _server_h
#define _server_h

#include "list.h"
#define FREE 0
#define BUSY 1
/*#define CREATE_LOCK 0
#define CREATE_CONDITION 1
#define DESTROY_LOCK 2
#define DESTROY_CONDITION 3
#define ACQUIRE 4
#define RELEASE 5
#define WAIT 6
#define SIGNAL 7
#define BROADCAST 8
#define CREATE_MV 9
#define DESTROY_MV 10
#define GET_MV 11
#define SET_MV 12*/
#define MAX_MVS 1000

int nextMvLocation = 0;


int serverCreateLock(char *buf, int netAddr, int mailBoxNum);
int serverCreateCondition(char *buf, int netAddr, int mailBoxNum);
int serverDestroyLock(int);
int serverDestroyCondition(int);
int serverAcquireLock(int lockId, int idNum[]);
int serverAcquireLock2(int lockId, int locMachineId, int mailbox);
int serverReleaseLock(int lockId);
int serverReleaseLock2(int lockId, int forwardServerId);
int serverReleaseLock3(int lockId);
int serverWait(int, int, int idNum[]);
int serverSignal(int, int, int idNum[]);
int serverWait2(int, int, int, int, int forwardServerId);
int serverSignal2(int, int);
int serverBroadcast(int, int);
int serverCreateMv(char*, int, int, int);
int serverDestroyMv(int);
int serverGetMv(int, int);
int serverSetMv(int, int, int);
void Server();


struct ServerLockOwnerStruct{
    int netAddr;
    int mailBoxNum;
    
};

struct ServerConditionOwnerStruct{
    int netAddr;
    int mailBoxNum;
    
};

typedef struct ServerLockOwnerStruct ServerLockOwner;
typedef struct ServerConditionOwnerStruct ServerConditionOwner;

class ServerLock{
private:
    char *name;
    int lockId;
    ServerLockOwner owner;
    List *lockWaitQueue;
    int serverLockState;
    
    
public:
    ServerLock(char *myName);
    ~ServerLock();
    bool isHeldByMe(int netAddress, int mailBoxNumber);
    void listAppend(char *msg);
    void listPrepend(char *msg);
    void *listRemove();
    bool isEmpty();
    char *getName(){ return name; }
    void setOwner(int netAddr, int mailBoxNumber){ 
        owner.netAddr = netAddr;
        owner.mailBoxNum = mailBoxNumber;
    }
    void setNet(int netAddr) { owner.netAddr = netAddr; }
    void setBox(int mailbox) { owner.mailBoxNum = mailbox; }
    
    int getNet(){
        return owner.netAddr;
    }
    int getBox(){
        return owner.mailBoxNum;
    }
    int getStatus(){ return serverLockState; }
    void setStatus(int status){ serverLockState = status; }
    
    
    
};


class ServerCondition{
private:
    List *cvWaitQueue;
    int serverLockId;
    char *name;
    ServerConditionOwner owner;
    
    
public:
    int peopleWaiting;
    ServerCondition(char *myName);
    ~ServerCondition();
    void setLockId(int idNum) { serverLockId = idNum; }
    int getLockId(){ return serverLockId; }
    void listAppend(char *msg);
    void *listRemove();
    bool isEmpty();
    char *getName(){ return name; }
    bool isHeldByMe(int netAddr, int mailBoxNum);
    void setOwner(int netAddr, int mailBoxNumber){ 
        owner.netAddr = netAddr;
        owner.mailBoxNum = mailBoxNumber;
    }
    void setNet(int netAddr) { owner.netAddr = netAddr; }
    void setBox(int mailbox) { owner.mailBoxNum = mailbox; }
    int getNet(){
        return owner.netAddr;
    }
    int getBox(){
        return owner.mailBoxNum;
    }
    List *getList(){
        return cvWaitQueue;
    }
    
    
    
};

struct ServerLockItem{
    ServerLock *lock;
    bool isDeleted;
    bool isToBeDeleted;
    int usageCount;
};

struct ServerConditionItem{
    ServerCondition *condition;
    bool isDeleted;
    bool isToBeDeleted;
    int usageCount;
};

struct ServerMvItem{
    bool isDeleted;
    int usageCount;
    int *mvArray;
    char *name;
    int arraySize;
};

typedef ServerLockItem ServerLockEntry;
typedef ServerConditionItem ServerConditionEntry;
typedef ServerMvItem ServerMvEntry;


#endif

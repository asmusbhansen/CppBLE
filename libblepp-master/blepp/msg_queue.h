#ifndef _MSGQUEUE_H_
#define _MSGQUEUE_H_

#include <iostream>
#include <deque>
#include <pthread.h>

enum PROCESS_ID{CHAR_1 = 0, CHAR_2 = 1};
enum EVENT_ID{NO_NEW_MESSAGE = 0, NEW_NOTIFICATION = 1};

class MsgQueue;


//Declare a struct to handle message and event ID
struct Message {

	PROCESS_ID senderID_;
	EVENT_ID eventID_;
	int val;
	double time;
};


class MsgQueue {

public:
	MsgQueue(int MaxSize = 1000); //Class constructor, takes queue max size as argument.
	~MsgQueue();
	void send(Message * _msg);	//Function to send messages, taking message pointer as argument.
	Message receive();	//Receive function, returns the message.

private:
	std::deque<Message>* mq;	//STL container double ended queue is used.
	int maxSize, numMsg;
	pthread_mutex_t editMutex;
	pthread_cond_t notFull, notEmpty;
};

#endif

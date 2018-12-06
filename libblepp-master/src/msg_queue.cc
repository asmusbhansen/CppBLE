#include "blepp/msg_queue.h"


MsgQueue::MsgQueue(int _maxSize) {

	pthread_mutex_init(&editMutex, 0);
	pthread_cond_init(&notFull, 0);
	pthread_cond_init(&notEmpty, 0);

	mq = new std::deque<Message>;
	mq -> resize(0);
	maxSize = _maxSize;
	numMsg = 0;

}

MsgQueue::~MsgQueue() {

	delete mq;	//The destructor deletes the message queue to avoid memory leak.
}

void MsgQueue::send(Message* _msg) {	//The send function takes the sender ID and message pointer(Message contains ).

	pthread_mutex_lock(&editMutex);	//When a process is sending a message to the message queue, only that process can edit the queue.

	while(numMsg == maxSize) {

		pthread_cond_wait(&notFull, &editMutex);	//If the queue is full, the process waits for signalling. Other processes can still work in the meantime.
								//The mutex is unlocked for other processes to use, and is returned when the coditional wait is over.
	}

	Message tempMsg = *_msg;
	mq -> push_back(tempMsg);	//The message item is pushed to the message queue.
	numMsg++;
	pthread_cond_signal(&notEmpty); //Other threads are signalled that the message queue is no longer empty.
	//std::cout << "Message sent with eID: " << tempMsg.eventID_ << " and pID : " << tempMsg.senderID_ << std::endl; 
	pthread_mutex_unlock(&editMutex); //The edit mutex is released before exiting the routine.
}

Message MsgQueue::receive() {

	pthread_mutex_lock(&editMutex);

	while(numMsg == 0) {
		pthread_cond_wait(&notEmpty, &editMutex);	//Same behaviour as above.
	}

	Message tempMsg = mq->front();	//The front item of the message queue is read.
	//std::cout << "Message read with eID: " << tempMsg.eventID_ << " and pID: " << tempMsg.senderID_ << std::endl;

	mq -> pop_front();	//The front item of the message queue is removed.
	numMsg--;
	pthread_cond_signal(&notFull);
	pthread_mutex_unlock(&editMutex);
	return tempMsg;

}

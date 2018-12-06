/*
 *
 *  libattgatt - Implementation of the Generic ATTribute Protocol
 *
 *  Copyright (C) 2013, 2014 Edward Rosten
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include <iostream>
#include <sstream>
#include <iomanip>
#include <blepp/blestatemachine.h>
#include <blepp/float.h>
#include <deque>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include "cxxgplot.h"  //lolzworthy plotting program
#include "blepp/csv_class.h"
#include "blepp/msg_queue.h"

using namespace std;
using namespace BLEPP;


#define ANGLE_SER 	UUID("8efc0000-6ee9-4c70-8615-3456c364d7e6")
#define ROLL_CHAR 	UUID("8efc0001-6ee9-4c70-8615-3456c364d7e6")
#define PITCH_CHAR 	UUID("8efc0002-6ee9-4c70-8615-3456c364d7e6")

//Define message queue
MsgQueue log_queue(1000);

//Log file
std::string log_file_name = "csv_file.csv";

void bin(uint8_t i)
{
	for(int b=7; b >= 0; b--)
		cout << !!(i & (1<<b));

}

//ASCII throbber
string throbber(int i)
{
	string base = " (--------------------)";
	
	i = i%40;
	if(i >= 20)
		i = 19-(i-20);
	base[i + 2] = 'O';

	return base + string(base.size(), '\b');
}


double get_time_of_day()
{
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec+tv.tv_usec * 1e-6;
}

int convert_16bit_value(const uint8_t * d)
{
	int val = ((d[1] << 8) + d[0]);
		
	//Format the points and send the results to the plotting program.
	if(val > 0b1<<15){
		val = val - (0b1 << 16);
	}
	
	return val - 90;
}

void add_to_readings(deque<int>* values, int val)
{
	values->push_back(val);
	if(values->size() > 300)
		values->pop_front();
}


//Thread function for logging value to csv file
void* log_to_csv(void* _arg) {

	//CSV file for logging
	csv_class csv(log_file_name,",");
	
	PROCESS_ID pID = *(PROCESS_ID*)_arg;	//(int*) casts the void pointer to an integer pointer, the '*' then returns the value of the integer pointer.

	std::string string_senderID;
	std::string string_value;
	std::string string_time;

	Message recvMsg;

	EVENT_ID eventID;
	PROCESS_ID senderID;
	int value;
	double time;
	
	std::cout << "Thread " << pID << " running" << std::endl;


	while(1)
	{
	recvMsg = log_queue.receive();
	eventID = recvMsg.eventID_;
	senderID = recvMsg.senderID_;
	value = recvMsg.val;
	time = recvMsg.time;
	
		if(eventID == NEW_NOTIFICATION)
		{
			std::vector<std::string> data_line;
			
			//Write senderID and value as line to csv file
			string_senderID = to_string(senderID);
			string_value = to_string(value);
			string_time = to_string(time);
		
			data_line.push_back(string_senderID);
			data_line.push_back(string_time);
			data_line.push_back(string_value);
			
			if(csv.write_csv(&data_line))
			{
				std::cout << "Error writing to csv file" << std::endl;
			}

			std::cout << "New msq from char " << senderID << " with value " << value << std::endl;

		}
	}

}

//Thread function for plotting value
void* plot_val(void* _arg) 
{

	//This is a cheap and cheerful plotting system using gnuplot.
	//Ignore this if you don't care about plotting.
	cplot::Plotter plot;
	plot.range = " [ ] [:] ";
	
	
	PROCESS_ID pID = *(PROCESS_ID*)_arg;	//(int*) casts the void pointer to an integer pointer, the '*' then returns the value of the integer pointer.

	Message recvMsg;

	EVENT_ID eventID;
	PROCESS_ID senderID;
	int value;
	double time;
	
	deque<int> values_deq;
	
	std::cout << "Thread " << pID << " running" << std::endl;

	while(1)
	{
	recvMsg = log_queue.receive();
	eventID = recvMsg.eventID_;
	senderID = recvMsg.senderID_;
	value = recvMsg.val;
	time = recvMsg.time;
	
		if(eventID == NEW_NOTIFICATION)
		{
			add_to_readings(&values_deq, value);
			
			plot.newline("line lw 3 lt 1 title \"\"");
			plot.addpts(values_deq);
			ostringstream os;
			//os << "set title \"Angle: " << 1 << " Number of values: " << count << "\"";
			plot.add_extra(os.str());

			plot.draw();

			std::cout << "New msq from char " << senderID << " with value " << value << std::endl;

		}
	}

}

////////////////////////////////////////////////////////////////////////////////
//
// This program demonstrates the use of the library
// 
int main(int argc, char **argv)
{
	if(argc != 2 && argc != 3)
	{	
		cerr << "Please supply address.\n";
		cerr << "Usage:\n";
		cerr << "prog address [nonblocking]\n";
		exit(1);
	}

	log_level = Info;

	
	//This is the interface to the BLW protocol.
	BLEGATTStateMachine gatt;


	fd_set write_set, read_set;
	

	//deque<int> values_roll;
	//deque<int> values_pitch;
	
	int count = -1;
	double start_time = 0;
	float voltage=0;
	
	//TO DO: Initialize message queue to pass messages from notification handlers to csv writer and plotter
	
	pthread_t log_csv_thread, plot_val_thread;
	int tID[2] = {1, 2};
	
	
	
	//Lambda for processing write and write requests
	std::function<void()> process_read_write = [&]() 
	{ 
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 10000;
		
		int result = select(gatt.socket() + 1, &read_set, &write_set, NULL, & tv);

		//Checks if the sockets are ready for read or write
		if(FD_ISSET(gatt.socket(), &write_set)) {
			gatt.write_and_process_next();
			cout << "Writing and processing next!" << endl;
		}

		if(FD_ISSET(gatt.socket(), &read_set)) {
			cout << "Reading and processing next!" << endl;
			gatt.read_and_process_next();
		}

	};
	
	
	////////////////////////////////////////////////////////////////////////////////	
	//
	// This is important! This is an example of a callback which responds to 
	// notifications or indications. Currently, BLEGATTStateMachine responds 
	// automatically to indications. Maybe that will change.
	//
	//Function that reads an indication and formats it for plotting.
	
	//Calback for roll characteristic notification
	/*Lambda recall:    Capture list [] - These variables are copied from outside. = means all, & means all by reference
						Arugment list () - These variables are passed to the lambda function at execution time
		*/			
	std::function<void(const PDUNotificationOrIndication&)> notify_cb_roll = [&](const PDUNotificationOrIndication& n)
	{
		Message sentMsg;
		
		double t;
		const uint8_t* d;
		int val;
		
		count++;
		
		t = get_time_of_day()-start_time;
		
		cout << t << " Seconds since first package\n";
		
		d = n.value().first;
		
		val = convert_16bit_value(d);
				
		//add_to_readings(&values_roll, val);
		
		cout << "Roll value: " << val << endl;
		/*
		//Plotting function - Should be in seperate thread
		plot.newline("line lw 3 lt 1 title \"\"");
		plot.addpts(values_roll);
		ostringstream os;
		os << "set title \"Angle: " << val << " Number of values: " << count << "\"";
		plot.add_extra(os.str());

		plot.draw();
		//End of plotting function
		*/
		//Send new notification value to csv log
		sentMsg.eventID_ = NEW_NOTIFICATION;
		sentMsg.senderID_ = CHAR_1;
		sentMsg.val = val;
		sentMsg.time = t;
		
		log_queue.send(&sentMsg);
		
	};
	
	//Callback function for pitch notifications
	std::function<void(const PDUNotificationOrIndication&)> notify_cb_pitch = [&](const PDUNotificationOrIndication& n)
	{
		Message sentMsg;
		
		double t;
		const uint8_t* d;
		int val;
		//std::vector<std::string> data_line;
		//std:string string_value;
		
		count++;
		
		t = get_time_of_day()-start_time;
		
		cout << t << " Seconds since first package\n";
		
		d = n.value().first;
		
		val = convert_16bit_value(d);
				
		//add_to_readings(&values_pitch, val);
		
		cout << "Pitch value: " << val << endl;

		//Send new notification value to csv log
		sentMsg.eventID_ = NEW_NOTIFICATION;
		sentMsg.senderID_ = CHAR_2;
		sentMsg.val = val;
		sentMsg.time = t;
		
		log_queue.send(&sentMsg);
			
	};

	//Function for subscribing to certain services and features in the BLE device
	bool enable=true;
	std::function<void()> cb = [&gatt, &notify_cb_roll, &notify_cb_pitch, &enable, &process_read_write](){

		fd_set write_set, read_set;

		FD_ZERO(&write_set);

		//The GATT socket is added to the read set
		FD_SET(gatt.socket(), &read_set);
		
		//The GATT socket is added to the write set if we want to wait for writes
		if(gatt.wait_on_write())
			FD_SET(gatt.socket(), &write_set);

		pretty_print_tree(gatt);

		for(auto& service: gatt.primary_services)
			for(auto& characteristic: service.characteristics) {
				
				
				if(service.uuid == ANGLE_SER && characteristic.uuid == ROLL_CHAR)
				{
					cout << "Found and subscribed to roll characteristic\n";
					characteristic.cb_notify_or_indicate = notify_cb_roll;
					characteristic.set_notify_and_indicate(enable, false);
				
					process_read_write();
				}
				else if(service.uuid == ANGLE_SER && characteristic.uuid == PITCH_CHAR)
				{
					
					cout << "Found and subscribed to pitch characteristic\n";
					characteristic.cb_notify_or_indicate = notify_cb_pitch;
					characteristic.set_notify_and_indicate(enable, false);
					
					process_read_write();

				}

			}

	};
	
	////////////////////////////////////////////////////////////////////////////////
	//
	// This is somewhat important.  Set up callback for disconnection
	//
	// All reasonable errors are handled by a disconnect. The BLE spec specifies that
	// if the device sends invalid data, then the client must disconnect.
	//
	// Failure to connect also comes here.
	gatt.cb_disconnected = [](BLEGATTStateMachine::Disconnect d)
	{
		cerr << "Disconnect for reason " << BLEGATTStateMachine::get_disconnect_string(d) << endl;
		exit(1);
	};
	
	//TO DO: Start threads for writing to csv_file and plotting
	if( 0 > pthread_create(&log_csv_thread, NULL, &log_to_csv, (void*)&tID[0]))
	{
		cout << "Thread creation failed" << endl;
		exit (EXIT_FAILURE);
	}

	if( 0 > pthread_create(&plot_val_thread, NULL, &plot_val, (void*)&tID[1]))
	{
		cout << "Thread creation failed" << endl;
		exit (EXIT_FAILURE);
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	// You almost always want to query the tree of things on the entire device
	// So, there is a function to do this automatically. This is a helper which
	// sets up all the callbacks necessary to automate the scanning. You could 
	// reduce connection times a little bit by only scanning for soma attributes.
	gatt.setup_standard_scan(cb);

	//Set start time for logging
	start_time = get_time_of_day();
	
	////////////////////////////////////////////////////////////////////////////////
	//
	// There are two modes, blocking and nonblocking.
	//
	// Blocking is useful for simple commandline programs which just log a bunch of 
	// data from a BLE device.
	//
	// Nonblocking is useful for everything else.
	// 
	
	// A few errors are handled by exceptions. std::runtime errors happen if nearly fatal
	// but recoverable-with-effort errors happen, such as a failure in socket allocation.
	// It is very unlikely you will encounter a runtime error.
	//
	// std::logic_error happens if you abuse the BLEGATTStateMachine. For example trying
	// to issue a new instruction before the callback indicating the in progress one has 
	// finished has been called. These errors mean the program is incorrect.
	try
	{ 
		if(argc >2 && argv[2] == string("nonblocking"))
		{
			
			//This is how to use the blocking interface. It is very simple.
			gatt.connect_blocking(argv[1]);
			for(;;)
			{
				gatt.read_and_process_next();
			}
		}
		else
		{
			//Connect as a non blocking call
			cout << "Trying to connect" << endl;
			gatt.connect_nonblocking(argv[1]);
			cout << "Probably connected?" << endl;

			//Example of how to use the state machine with select()
			//
			//This just demonstrates the capability and should be easily
			//transferrable to poll(), epoll(), libevent and so on.
			//fd_set write_set, read_set;

			for(int i=0;;i++)
			{
				//Set the read and write monitor sets to zero
				FD_ZERO(&read_set);
				FD_ZERO(&write_set);

				//Reads are always a possibility due to asynchronus notifications.
				//The GATT socket is added to the read set
				FD_SET(gatt.socket(), &read_set);
				
				//Writes are usually available, so only check for them when the 
				//state machine wants to write.
				//The GATT socket is added to the write set if we want to wait for writes
				if(gatt.wait_on_write())
					FD_SET(gatt.socket(), &write_set);
					
				
				struct timeval tv;
				tv.tv_sec = 0;
				tv.tv_usec = 10000;
				
				//Selects blocks until one of the monitored sockets are ready for either read or write. The read_set and write_set will contain the sockets ready for either read or write
				int result = select(gatt.socket() + 1, &read_set, &write_set, NULL, & tv);

				//Checks if the sockets are ready for read or write
				
				if(FD_ISSET(gatt.socket(), &write_set))
					gatt.write_and_process_next();

				if(FD_ISSET(gatt.socket(), &read_set)) {
					gatt.read_and_process_next();
				}
				cout << throbber(i) << flush;
/*
				if(i % 100 == 0 && gatt.is_idle())
				{
					enable = !enable;
					cb();
				}*/

			}
		}
	}
	catch(std::runtime_error e)
	{
		cerr << "Something's stopping bluetooth working: " << e.what() << endl;
	}
	catch(std::logic_error e)
	{
		cerr << "Oops, someone fouled up: " << e.what() << endl;
	}

}

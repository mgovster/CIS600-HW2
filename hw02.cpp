/*
HW2_Govia_magovia.txt
SU Net ID: magovia   SU ID: 644471631

Code by: Matt Govia 

problems:
-The updated buffer would take the contents of another, or not update correctly at all
-If the whole function was locked with wait time also locked, I think it would have went better
-the updated buffer and assemblyLine's would work as expected in 
*/
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <fstream>
#include <chrono>

#define us = std::chrono_literals::us;
#define ns = std::chrono_literals::ns;
using namespace std;
mutex mtx;
condition_variable cv;
bool ready{ false };
ofstream file;
int Seed = time(0);
string fileName{ "log.txt" };
chrono::steady_clock::time_point programStart; //chrono::system_clock::time_point 
  //Having ^ up here makes time negative at start(if initialized up here), starting in main is closest to 0
int buffer[4];
//So buffer is shared resource, make it global and be able to lock it in each funct.	
int totalProductsCompleted = 0;
chrono::steady_clock::time_point worker_time;

void PartWorker(int id) {
	unique_lock<mutex> lck(mtx);
	//cv.wait(lck, [] {return ready; });
	auto currtime = chrono::steady_clock::now();
	chrono::microseconds totalWaitTime(0);		//when implementing the wait_until command to see if
	//worker has not surpassed the time limit, if they did not, loop back and this time will be the time
	//of that current worker minus the previous one (might need a temp time to remember last one)
	//totalWaitTime = duration<int> (workerTime - firstWorker)
	//where workerTime is new steady clock at the start of the loop
	//currtime is the worker's start time

	chrono::steady_clock::duration elapse = currtime - programStart;
	chrono::microseconds melapse = chrono::duration_cast<chrono::microseconds> (elapse);
	file << "\n";
	//file << "Current Worker Time: " << melapse.count() << "us\n";
	cv.notify_one();
	//file << "Part Worker ID: " << id << "\n";
	srand(Seed++);
	int assemblyLimit;

	//create buffer
	int buff[4];
	int lim = 6, c = 0;
	for (int i = 0; i < sizeof(buff) / 4; i++) {
		buff[i] = rand() % lim;
	}
	for (int e : buff) { buffer[c] = e; c++; }

	int assemblyLine[]{ 0,0,0,0 };
	int pos = 0;
	int iteration = 1, randnum;

	chrono::steady_clock::time_point first_worker = chrono::steady_clock::now();
	//instead of first_worker, I could have used currtime
	chrono::steady_clock::time_point next_worker;
	chrono::steady_clock::duration it;
	chrono::microseconds it_time;
	while (iteration <= 5) {
		srand(Seed++);
		next_worker = chrono::steady_clock::now();
		it = next_worker - programStart;
		it_time = chrono::duration_cast<chrono::microseconds>(it);
		file << "Current worker time: " << it_time.count() << "\n";
		file << "Part Worker ID: " << id << "\n";
		file << "Iteration: " << iteration << "\n";
		int parts = 4;
		file << "Status: " << "New Load Order" << "\n";
		while (parts > 0) {
			randnum = rand() % 2;		//rand int from 0 to 1
			parts -= randnum;
			assemblyLine[pos] += randnum;
			pos++;
			if (pos == 4) { pos = 0; }
		}
		if (iteration == 1) { next_worker = first_worker; }
		//accumulated wait time is for when iterations loop arround, the wait time will add up
		totalWaitTime = chrono::duration_cast<chrono::duration<long>>(next_worker - first_worker);
		file << "Accumulated Wait Time: " << totalWaitTime.count() << "us\n";

		//I have the parts, now I have to get the time it takes (50,70,90,110) us 
		chrono::microseconds assemblyTime(0);
		assemblyTime += chrono::microseconds(50) * assemblyLine[0] + chrono::microseconds(70) * assemblyLine[1] +
			chrono::microseconds(90) * assemblyLine[2] + chrono::microseconds(110) * assemblyLine[3];
		//Assembly's time got, now time for buffer loading time 

		chrono::microseconds buffTime(0);
		buffTime += chrono::microseconds(20) * buffer[0] + chrono::microseconds(30) * buffer[1] + chrono::microseconds(40) * buffer[2]
			+ chrono::microseconds(50) * buffer[3];

		chrono::microseconds totalTimeToWait = assemblyTime + buffTime;

		file << "Buffer State: (";
		for (int e = 0; e < sizeof(buffer) / 4; e++) {
			if (e != 3) { file << buffer[e] << ","; }
			else { file << buffer[e]; }
		}
		file << ")\n";

		file << "Load Order: (";
		for (int e = 0; e < sizeof(assemblyLine) / 4; e++) {
			if (e != 3) { file << assemblyLine[e] << ","; }
			else { file << assemblyLine[e]; }
		}
		file << ")\n";

		//might have to add a lock guard at this chunk up until updated buffer n such
		//it sometimes breaks into a product worker in the middle of the thread

		//part worker will now wait at buffer for next set of instructions for 3000 microseconds max
		bool full = true;
		int limit = 6;
		int newBuffer[4] = {}, newAssemblyLine[4] = {};
		for (int e : buffer) { if (limit != e) { full = false; limit--;}  }	//checks if buffer is full for wait 
		/*if (full) {
			cv.wait(lck);
		}*/ //if I have to have the worker wait because the buffer is full ^	
		auto curr = chrono::system_clock::now();
		if (cv.wait_until(lck, curr + chrono::microseconds(3000)) == std::cv_status::timeout) {
			//I'm pretty sure 
			if (!full) {	//if it is not full -> add worker's stuff to it
				//now add the assembled parts into the buffer up unto the max of each (6,5,4,3) repectively
				assemblyLimit = 6;
				//assembly line and buffer
				int temp;
				for (int i = 0; i < sizeof(buffer) / 4; i++) {		//might have to fix later either here or above
					//later on in iterations, assembly line becomes negative
					temp = 0;
					if (buffer[i] == assemblyLimit) {
						temp = buffer[i];
						newBuffer[i] = temp;
						newAssemblyLine[i] = assemblyLine[i];
					}
					else if (buffer[i] + assemblyLine[i] > assemblyLimit) {
						temp = (buffer[i] + assemblyLine[i]) - assemblyLimit;
						newBuffer[i] = assemblyLimit;
						newAssemblyLine[i] += temp;
					}
					else {
						temp = buffer[i] + assemblyLine[i];
						newBuffer[i] = temp;
						newAssemblyLine[i] = 0;
					}

					assemblyLimit--;
				}
				int n = 0;
				for (int i : newBuffer) { buffer[n] = i; n++; }
				n = 0;

				file << "Updated Buffer State: (";
				for (int e = 0; e < sizeof(buffer) / 4; e++) {
					if (e != 3) { file << buffer[e] << ","; }
					else { file << buffer[e]; }
				}
				file << ")\n";

				file << "Updated Load Order: (";
				for (int e = 0; e < sizeof(newAssemblyLine) / 4; e++) {
					if (e != 3) { file << newAssemblyLine[e] << ","; }
					else { file << newAssemblyLine[e]; }
				}
				file << ")\n";
			}
			else {		//no more room, discard worker's amount and add the time together
				//oh also end the iteration cause buffer is full
				assemblyTime = chrono::milliseconds(0);
				assemblyTime += chrono::microseconds(20) * newAssemblyLine[0] + chrono::microseconds(30) * newAssemblyLine[1] +
					chrono::microseconds(40) * newAssemblyLine[2] + chrono::microseconds(50) * newAssemblyLine[3];
				totalTimeToWait += assemblyTime;

				
				file << "Status: " << "Time-out occured" << "\n";
				//cv.notify_one();
				iteration += 100;	//to end iteration loop
			}
			full = true;
			for (int e : buffer) { if (limit != e) { full = false; limit--; } }	//checks if buffer is full for wait 

		}
		else {
			//this might be where completed packages go
			//or might be when there is no timeout, so maybe cv.wait?
			//cv.wait(lck);
			totalWaitTime += chrono::microseconds(6000);
			file << "Updated Buffer State: (";
			for (int e = 0; e < sizeof(buffer) / 4; e++) {
				if (e != 3) { file << buffer[e] << ","; }
				else { file << buffer[e]; }
			}
			file << ")\n";

			file << "Updated Load Order: (";
			for (int e = 0; e < sizeof(assemblyLine) / 4; e++) {
				if (e != 3) { file << assemblyLine[e] << ","; }
				else { file << assemblyLine[e]; }
			}
			file << ")\n";
		}
		
		//part worker will now wait for time required to move and build parts

		//this_thread::sleep_for(totalTimeToWait);
		next_worker = chrono::steady_clock::now();

		file << "\n\n";
		//std::this_thread::sleep_for(chrono::duration<chrono::microseconds>(totalTimeToWait));
		iteration++;
	}
	cv.notify_one();
	file << "\n";
}




void ProductWorker(int id) {
	//lock_guard<mutex> lock2(mtx);
	unique_lock<mutex> lock2(mtx);
	chrono::microseconds totalWaitTime(0);
	auto currtime = chrono::steady_clock::now();
	chrono::steady_clock::duration elapse = currtime - programStart;
	chrono::microseconds melapse = chrono::duration_cast<chrono::microseconds> (elapse);
	totalWaitTime -= totalWaitTime;		//reset it back to 0
	file << "\n";
	file << "Current Worker Time: " << melapse.count() << "us\n";
	file << "Product Worker ID: " << id << "\n";
	int iteration = 1;
	int productLine[] = { 0,0,0,0 };
	int parts;
	int randnum;
	int pos = 0;
	int completedPackages = 0;
	//Create buffer


	chrono::steady_clock::time_point first_worker = chrono::steady_clock::now();
	chrono::steady_clock::time_point next_worker;
	worker_time = first_worker;
	while (iteration <= 5) {
		srand(Seed++);
		next_worker = chrono::steady_clock::now();
		parts = 5;
		//product worker loop (containing 5 parts total)
		file << "Iteration: " << iteration << "\n";
		file << "Status: " << "New Pickup Order \n";

		/*Plan for parts:
		pick a random number from 0 to 3, this will be the pos that we will ignore
		add random int from 0 to 1
		*/
		int ignoreMe = rand() % 4; //rand int from 0 to 3
		while (parts > 0) {
			if (ignoreMe != pos) {
				randnum = rand() % 2;	//0 to 1
				parts -= randnum;
				productLine[pos] += randnum;
			}
			
			pos++;
			if (pos >= 4) { pos = 0; }
		}

		//time to calculate time it'll take 
		//chrono::microseconds::duration

		if (iteration == 1) { next_worker = first_worker; }
		//accumulated wait time is for when iterations loop arround, the wait time will add up
		chrono::duration<long> wait_time = chrono::duration_cast<chrono::duration<long>>(next_worker - worker_time);
		totalWaitTime += wait_time;
		file << "Accumulated Wait Time: " << totalWaitTime.count() << "us\n";

		
		chrono::microseconds dissassemblyTime(0);	//for discard time
		int newBuffer[4], newProductLine[4];
		//productLine[i] - buffer[i]
		
		//I have the parts, now I have to get the time it takes (80,100,120,140) us 
		chrono::microseconds assemblyTime(0);
		assemblyTime += chrono::microseconds(80) * buffer[0] + chrono::microseconds(100) * buffer[1] +
			chrono::microseconds(120) * buffer[2] + chrono::microseconds(140) * buffer[3];
		//Assembly's time got, now time for buffer loading time 

		chrono::microseconds moveTime(0);
		moveTime += chrono::microseconds(20) * productLine[0] + chrono::microseconds(30) * productLine[1] + chrono::microseconds(40) * productLine[2]
			+ chrono::microseconds(50) * productLine[3];

		chrono::microseconds totalTimeToWait = assemblyTime + moveTime;

		//file << "Accumulated Wait Time: " << totalTime;
		file << "Buffer State: (";
		for (int e = 0; e < sizeof(buffer) / 4; e++) {
			if (e != 3) { file << buffer[e] << ","; }
			else { file << buffer[e]; }
		}
		file << ")\n";

		file << "Load Order: (";
		for (int e = 0; e < sizeof(productLine) / 4; e++) {
			if (e != 3) { file << productLine[e] << ","; }
			else { file << productLine[e]; }
		}
		file << ")\n";

		//we have: buffer[], productLine[]
		//now that I have the parts, I need to move parts[i] out of buffer and calculate that time
		//where in this case i is the position
		int temp;
		chrono::steady_clock::time_point curr = chrono::steady_clock::now();
		bool usable = false;
		int limit = 0;
		for (int i = 0; i < sizeof(newBuffer) / 4; i++) {
			newBuffer[i] = 0;
			newProductLine[i] = 0;
		}
		//check if can take out of buffer any more
		for (int i = 0; i < sizeof(buffer) / 4; i++) {
			if (buffer[i] - productLine[i] > 0) { usable = true; }
		}
		ready = usable;

		int n = 0;
		for (int i : newBuffer) { buffer[n] = i; n++; }		//I added the updated buffer and pickup order here because
		n = 0;												//in the times I would run it, several times where
		file << "Updated Buffer State: (";					//after printing the load order, another thread would be printing
		for (int i = 0; i < sizeof(buffer) / 4; i++) {		//This could be because of the wait_until command I added
			if (i != 3) { file << buffer[i] << ","; }		//It could have slept the current thread and continued on
			else { file << buffer[i] << ")\n"; }			
		}

		file << "Updated Pickup Order: (";
		for (int i = 0; i < sizeof(newProductLine) / 4; i++) {
			if (i != 3) { file << newProductLine[i] << ","; }
			else { file << newProductLine[i] << ")\n"; }
		}

		if (cv.wait_until(lock2, curr + chrono::microseconds(6000)) != std::cv_status::timeout) {
			//at this point usually other threads insert in
			//check if either:
			//can take more from buffer or if there's more room to take more
			if (ready) {
				for (int i = 0; i < sizeof(productLine) / 4; i++) {
					temp = 0;
					if (buffer[i] < productLine[i]) {
						newBuffer[i] = 0;
						newProductLine[i] = productLine[i] - buffer[i];
					}
					else if (buffer[i] > productLine[i]) {
						newBuffer[i] = buffer[i] - productLine[i];
						newProductLine[i] = 0;
					}
					else {
						newBuffer[i] = 0;
						newProductLine[i] = 0;
					}
				}
				totalProductsCompleted++;				//completed a set so add to total (might have to
														//be plus amount of parts 
				int n = 0;
				for (int i : newBuffer) { buffer[n] = i; n++; }		//buffer gets set to newBuffer
				n = 0;												//for next iteration to get new parts
				file << "Updated Buffer State: (";
				for (int i = 0; i < sizeof(buffer) / 4; i++) {
					if (i != 3) { file << buffer[i] << ","; }
					else { file << buffer[i] << ")\n"; }
				}

				file << "Updated Pickup Order: (";
				for (int i = 0; i < sizeof(newProductLine) / 4; i++) {
					if (i != 3) { file << newProductLine[i] << ","; }
					else { file << newProductLine[i] << ")\n"; }
				}
				file << "Total Completed Products: " << totalProductsCompleted << "\n\n";
			}
			else {
				//else dismantle amount of parts and end iterations
				dissassemblyTime = chrono::microseconds(0);
				dissassemblyTime = chrono::microseconds(20)* productLine[0] + chrono::microseconds(30) * productLine[1] + chrono::microseconds(40) * productLine[2]
					+ chrono::microseconds(50) * productLine[3];
				for (int a = 0; a < sizeof(productLine) / 4; a++) { productLine[a] = 0; }
				file << "Updated Buffer State: (";
				for (int i = 0; i < sizeof(buffer) / 4; i++) {
					if (i != 3) { file << buffer[i] << ","; }
					else { file << buffer[i] << ")\n"; }
				}

				file << "Not so Updated Pickup Order: (";
				for (int i = 0; i < sizeof(productLine) / 4; i++) {
					if (i != 3) { file << productLine[i] << ","; }		//nothing should be in here anymore cause it got removed
					else { file << productLine[i] << ")\n"; }
				}
				file << "Total Completed Products: " << totalProductsCompleted << "\n\n";

				iteration += 100;		//end the loops and begin a new worker
				//cv.notify_one();
			}


		}
		else {
			//this might be for wake-up notified
			file << "Status: Wakeup-Timeout\n";
			totalWaitTime += chrono::microseconds(6000);
			file << "Updated Buffer State: (";
			for (int e = 0; e < sizeof(buffer) / 4; e++) {
				if (e != 3) { file << buffer[e] << ","; }
				else { file << buffer[e]; }
			}
			file << ")\n";

			file << "Updated Load Order: (";
			for (int e = 0; e < sizeof(productLine) / 4; e++) {
				if (e != 3) { file << productLine[e] << ","; }
				else { file << productLine[e]; }
			}
			file << ")\n";

			iteration += 100;
		}
		
		iteration++;
		//set productLine to 0,0,0,0
		//for (int i = 0; i < sizeof(productLine) / 4; i++) { productLine[i] = 0; }

	}
	ready = true;
	cv.notify_one();
}

int main() {
	auto startT = chrono::steady_clock::now();
	programStart = startT;
	srand(Seed);
	const int m = 20, n = 16; //m: number of Part Workers
	//n: number of Product Workers
	//m>n
	thread partW[m];
	thread prodW[n];
	file.open(fileName);
	for (int i = 0; i < n; i++) {
		partW[i] = thread(PartWorker, i);
		prodW[i] = thread(ProductWorker, i);
		cout << i << endl;
	}
	for (int i = n; i < m; i++) {
		partW[i] = thread(PartWorker, i);
		cout << i << endl;
	}
	/* Join the threads to the main threads */
	for (int i = 0; i < n; i++) {
		partW[i].join();
		prodW[i].join();
	}
	for (int i = n; i < m; i++) {
		partW[i].join();
	}
	file.close();
	cout << "Finish!" << endl;
	return 0;
}

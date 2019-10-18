#ifndef MCS_LOCK_H
#define MCS_LOCK_H

#include <cstdint>
#include <assert.h>
#include <atomic>
#include <thread>

#include <iostream>
using namespace std;

namespace sync
{
	class Node
	{
		public:
		atomic<Node*> next={nullptr};
		atomic<bool> wait={false};
	};
	class mcs_lock 
	{
		private:
		atomic<Node*> tail = {NULL};
	
		public:
	
		inline void Enter(Node*);
	
		inline void Leave(Node*);
	};
}

#endif // MCS_LOCK_H
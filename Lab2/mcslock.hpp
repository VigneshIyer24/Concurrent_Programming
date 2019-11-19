

#ifndef MCS_LOCK_H
#define MCS_LOCK_H

#include <cstdint>
#include <assert.h>
#include <atomic>
#include <thread>

#include <iostream>
constexpr size_t CACHELINE_SIZE = 64;
using namespace std;

namespace sync {
  
    class mcs_lock {

        struct mcs_node {

            bool locked{true};
            uint8_t variable1[CACHELINE_SIZE - sizeof(bool)];

            mcs_node* next{nullptr};
            uint8_t variable2[CACHELINE_SIZE - sizeof(mcs_node*)];

        };

        static_assert(sizeof(mcs_node) == 2 * CACHELINE_SIZE, "");

    public:

        void Enter();

        void Leave();

    private:

        std::atomic<mcs_node*> tail{nullptr};
        static thread_local mcs_node local_node;

    };

}


#endif // MCS_LOCK_H
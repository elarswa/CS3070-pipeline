#include <condition_variable>
#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <sstream>
using namespace std;
#define NUM_PER_THREAD 1000

class Pipeline
{
    static queue<int> q;
    static queue<int> q2; //filtered queue
    static condition_variable q_cond, q2_cond;
    static mutex q_sync, q2_sync, print;
    static atomic_size_t nprod;
    static atomic_size_t nfilter;
    static ofstream output;

public:
    static const size_t nprods = 4U, nfilter3s = 3U, ngroups = 10U;

    static void writeOut(size_t threadNum)
    {
        ostringstream file;
        file << "group" << threadNum << ".out";
        ofstream out{file.str().c_str()};
        unsigned int total = 0U;
        for (;;)
        {
            //sync with q2
            unique_lock<mutex> q2lck(q2_sync);
            //wait until something to process
            q2_cond.wait(q2lck, []() { return !q2.empty() || !nfilter; });
            if (q2.empty() && !nfilter)
                break;
            //write to file
            auto x = q2.front();
            if (x % 10 == (int)threadNum)
            {
                q2.pop();
                out << x << endl;
                ++total;
            }
        }
        lock_guard<mutex> plck(print);
        cout << "Group " << threadNum << " has " << total << " numbers" << endl;
    }

    static void remove3s()
    {
        for (;;)
        {
            // Get lock for sync mutex
            unique_lock<mutex> qlck(q_sync);
            // Wait for queue to have something to process
            q_cond.wait(qlck, []() { return !q.empty() || !nprod; }); // if q is empty wait
            if (q.empty() && !nprod)
                break; // exit condition
            auto x = q.front();
            if (x % 3 == 0)
                q.pop(); // remove if multiple of 3
            else
            {
                q.pop();
                unique_lock<mutex> q2lck(q2_sync);
                q2.push(x);
                q2lck.unlock();
                q2_cond.notify_one();
            }
            qlck.unlock();
        }
        --nfilter;
        q2_cond.notify_all();
    }

    static void produce(int i)
    {
        // Generate 1000 random ints
        srand(time(nullptr) + i * (i + 1));
        for (int i = 0; i < NUM_PER_THREAD; ++i)
        {
            int n = rand(); // Get random int
            // Get lock for queue; push int
            unique_lock<mutex> qlck(q_sync);
            q.push(n);
            qlck.unlock();
            q_cond.notify_one();
        }
        // Notify consumers that a producer has shut down
        --nprod;
        q_cond.notify_all();
    }
};
// Define static class data members
queue<int> Pipeline::q;
queue<int> Pipeline::q2; //filtered queue
condition_variable Pipeline::q_cond, Pipeline::q2_cond;
mutex Pipeline::q_sync, Pipeline::q2_sync, Pipeline::print;
atomic_size_t Pipeline::nprod(nprods);
atomic_size_t Pipeline::nfilter(nfilter3s);

int main()
{
    // auto start = chrono::high_resolution_clock::now();
    vector<thread> prods, cons, printers;
    for (size_t i = 0; i < Pipeline::ngroups; ++i)
        printers.push_back(thread(&Pipeline::writeOut, i));
    for (size_t i = 0; i < Pipeline::nfilter3s; ++i)
        cons.push_back(thread(&Pipeline::remove3s));
    for (size_t i = 0; i < Pipeline::nprods; ++i)
        prods.push_back(thread(&Pipeline::produce, i));

    // Join all threads
    for (auto &p : prods)
        p.join();

    for (auto &c : cons)
        c.join();

    for (auto &c : printers)
        c.join();

    // auto stop = chrono::high_resolution_clock::now();
    // cout << chrono::duration<double>(stop - start).count() << endl;
}
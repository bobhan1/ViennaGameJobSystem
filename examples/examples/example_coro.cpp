

#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <functional>
#include <string>
#include <algorithm>
#include <chrono>


#include "VGJS.h"
#include "VGJSCoro.h"

using namespace std::chrono;


namespace coro {

    using namespace vgjs;

    auto g_global_mem4 = n_pmr::synchronized_pool_resource( { .max_blocks_per_chunk = 20, .largest_required_pool_block = 1 << 20 }, n_pmr::new_delete_resource());

    class CoroClass {
        int number = 1;
    public:
        CoroClass( int nr ) : number(nr) {};

        Coro<int> Number10() {
            //std::cout << "Number x 10 = " << number * 10 << "\n";
            co_return number * 10;
        }

    };

    Coro<int> recursive2(std::allocator_arg_t, n_pmr::memory_resource* mr, int i, int N) {
        if (i < N ) {
            n_pmr::vector<Coro<int>> vec{ mr };
            vec.emplace_back( recursive2(std::allocator_arg, mr, i + 1, N));
            vec.emplace_back(recursive2(std::allocator_arg, mr, i + 1, N));

            co_await vec;
        }
        co_return 0;
    }

    Coro<int> recursive(std::allocator_arg_t, n_pmr::memory_resource* mr, int i, int N) {
        //std::cout << "Recursive " << i << " of " << N << std::endl;

        if (i < N) {
            co_await recursive(std::allocator_arg, mr, i + 1, N);
            co_await recursive(std::allocator_arg, mr, i + 1, N);
        }
        co_return 0;
    }

    Coro<float> computeF(std::allocator_arg_t, n_pmr::memory_resource* mr, int i) {

        co_await thread_index_t{ 0 };

        float f = i + 0.5f;
        //std::cout << "ComputeF " << f << std::endl;

        co_return 10.0f * i;
    }


    Coro<int> compute(std::allocator_arg_t, n_pmr::memory_resource* mr, int i) {

        //co_await thread_index{1};

        //std::cout << "Compute " << i << std::endl;

        co_return 2 * i;
    }

    Coro<int> do_compute(std::allocator_arg_t, n_pmr::memory_resource* mr) {
        //std::cout << "DO Compute " << std::endl;

        auto vec = n_pmr::vector<Coro<int>>{ mr };
        vec.push_back(compute(std::allocator_arg, &g_global_mem4, 2));
        vec.push_back(compute(std::allocator_arg, &g_global_mem4, 3));

        auto vec2 = n_pmr::vector<Coro<int>>{ mr };
        vec2.push_back(compute(std::allocator_arg, &g_global_mem4, 4));
        vec2.push_back(compute(std::allocator_arg, &g_global_mem4, 5));

        auto [ret1, ret2, ret3, ret4] = co_await parallel(compute(std::allocator_arg, &g_global_mem4, 1), []() { volatile int x = 1; }, Function([]() { volatile int x = 1; }), vec, vec2, compute(std::allocator_arg, &g_global_mem4, 99));  //or std::move

        co_return 0;
    }

    void FCompute( int i ) {
        //std::cout << "FCompute " << i << std::endl;
    }

    void FuncCompute(int i) {
        //std::cout << "FuncCompute " << i << std::endl;
    }

    Coro<> loop(std::allocator_arg_t, n_pmr::memory_resource* mr, int count) {
        int sum = 0;

        bool printb = false;

        if(printb) std::cout << "Starting loop\n";

        CoroClass cc(99);

        auto tv = n_pmr::vector<Coro<int>>{mr};

        auto tk = std::make_tuple(n_pmr::vector<Coro<int>>{mr}, n_pmr::vector<Coro<float>>{mr});
        
        auto fv = n_pmr::vector<std::function<void(void)>>{ mr };

        n_pmr::vector<Function> jv{ mr };

        for (int i = 0; i < count; ++i) {
            tv.emplace_back( do_compute(std::allocator_arg, &g_global_mem4 ) );

            get<0>(tk).emplace_back(compute(std::allocator_arg, &g_global_mem4, i));
            get<1>(tk).emplace_back(computeF(std::allocator_arg, &g_global_mem4, i));

            fv.emplace_back([=]() { FCompute(i); });

            Function f([=]() { FuncCompute(i); }, thread_index_t{}, thread_type_t{ 0 }, thread_id_t{ 0 });
            jv.push_back( f );

            jv.push_back(Function([=](){ FuncCompute(i); }, thread_index_t{}, thread_type_t{ 0 }, thread_id_t{ 0 }));
        }
        
        if (printb) std::cout << "Before loop " << std::endl;

        auto mf = cc.Number10();

        co_await mf;

        if (printb) std::cout << "Class member function " << mf.get() << std::endl;

        co_await tv;

        co_await tk;

        co_await recursive2(std::allocator_arg, &g_global_mem4, 1, 10);

        if (printb) std::cout << "Before First FCompute 999\n";
         
        co_await [=]() { FCompute(999); };

        if (printb) std::cout << "After First FCompute 999\n";

        co_await Function([=]() { FCompute(999); });

        if (printb) std::cout << "After Second FCompute 999\n";

        co_await fv;

        co_await jv;

        if (printb) std::cout << "Ending loop " << std::endl;

        co_return;
    }


    Coro<int> coroTest(int i);

    std::atomic<uint32_t> cnt = 0;


    Coro<bool> coroTest2(int i) {
        if (i > 0) {
            n_pmr::vector<Coro<int>> ch;

            ch.push_back(coroTest(i));
            ch.push_back(coroTest(i));
            co_await ch;
        }
        co_return true;
    }

    Coro<float> coroTest1(int i) {
        //std::cout << "Before coroTest1() " << std::endl;
        co_await thread_index_t{ 1 };

        auto retObj = coroTest2(i);
        co_await retObj;

        //std::cout << "After coroTest1() " << std::endl;

        co_return 0.0f;
    }

    Coro<int> coroTest(int i) {
        cnt++;

        //std::cout << "Begin coroTest() " << std::endl;
        co_await coroTest1(i - 1)(thread_index_t{}, thread_type_t{ 0 }, thread_id_t{ 2 });
        //std::cout << "End coroTest() " << std::endl;

        co_return cnt;
    }

    Coro<int> yield_test(int &input_parameter) {
        int value = 0;
        while (true) {
            co_yield value * input_parameter;
            co_await coroTest(5);
            ++value;
        }
        co_return value;
    }

    int g_yt_in = 0;
    auto yt = yield_test(g_yt_in);

    JobQueue<Coro<int>> coro_queue;

    Coro<float> driver( int i) {

        coro_queue.push( &yt );

        //co_await thread_index{};

        auto* pyt = coro_queue.pop();

        g_yt_in = i;
        co_await *pyt;
        std::cout << "Yielding " << pyt->get() << "\n";

        int res = co_await coroTest(i);

        //auto ct = coroTest(i);  //this starts a new tree
        //ct.resume();


        co_await loop(std::allocator_arg, &g_global_mem4, i);


        std::cout << "End coroTest() " << cnt << std::endl;

        co_return 0.0f;
    }

	void test() {
        cnt = 0;
        std::cout << "Starting coro test()\n";

        //auto dr = driver(4);  //this starts a new tree
        //dr.resume();

        Coro<float> retObj = driver(4);  //this starts a new tree
        schedule(retObj);

        if(retObj.ready()) std::cout << "Result " << retObj.get() << std::endl;

        std::cout << "Ending coro test()\n";

	}

}

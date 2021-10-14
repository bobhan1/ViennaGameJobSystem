#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <functional>
#include <string>
#include <algorithm>
#include <chrono>


#include "VGJS.h"


using namespace std::chrono;


namespace func {

    using namespace vgjs;

    auto g_global_mem5 = ::n_pmr::synchronized_pool_resource({ .max_blocks_per_chunk = 100000, .largest_required_pool_block = 1 << 22 }, n_pmr::new_delete_resource());

    std::atomic<uint32_t> cnt = 0;

    void printData(int i);

    auto computeF(int i) {
        //std::this_thread::sleep_for(std::chrono::microseconds(1));

        //std::cout << "ComputeF " << i << "\n";

        return i * 10.0;
    }

    auto compute( int i ) {
        volatile auto x = 2 * i;
        
        schedule([=](){ printData(i - 1); });
        schedule([=](){ printData(i - 1); });

        continuation([=](){ computeF(i); });

        return x;
    }

    void printData( int i ) {
        //std::cout << "Print Data " << i << std::endl;
        if (i > 0) {
            cnt++;
            Function r{ [=]() { compute(i); } };
            schedule( r );
            //schedule( F( printData(i-1); ) );
            //schedule( F( printData(i-1); ));
        }
    }

    void loop(int N) {
        for (int i = 0; i < N; ++i) {
            schedule([=](){ compute(i); } );
        }
    }

    void driver(int i) {
        std::cout << "Driver " << i << std::endl;

        schedule(Function{ [=]() { printData(i); } });

        //schedule(F(loop(100000)));
    }


    void end() {
        std::cout << "Ending func test() " << cnt << "\n";
    }

    void test() {
        cnt = 0;
        std::cout << "Starting func test()\n";

        std::function<void(void)> f([]() { driver(13); }); // = std::bind( driver, 13 );

        schedule(f);

        continuation(end);
    }




}


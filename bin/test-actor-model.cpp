
#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/thread.hpp>
#include <mbgl/actor/actor_ref.hpp>
#include <mbgl/storage/default_file_source.hpp>

#include <cstdlib>
#include <iostream>
#include <csignal>
#include <atomic>
#include <sstream>
#include <chrono>
#include <thread>

using namespace mbgl;
using namespace mbgl::util;
using namespace std::literals::chrono_literals;

class A
{
public:
    A(){
        tid = std::this_thread::get_id();
        std::cout<<"A thread id "<<tid<<std::endl<<std::flush;
    }
    void add(int v)
    {
        assert(tid == std::this_thread::get_id());
        value += v;
        std::cout<<"A add "<<std::this_thread::get_id()<<" "<<value<<std::endl<<std::flush;
    }

private:
    int value=0;
    std::thread::id tid;
};

class B
{
public:
    B(){
        tid = std::this_thread::get_id();
        std::cout<<"B thread id "<<tid<<std::endl<<std::flush;

    }
    void do_calc(std::string s, ActorRef<A> b)
    {
        assert(tid == std::this_thread::get_id());
        std::cout<<"B do_calc "<<std::this_thread::get_id()<<" "<<s<<std::endl<<std::flush;
        b.invoke(&A::add,2);
    }

private:
    std::thread::id tid;
};

int main(int /*argc*/, char **/**argv[]*/) {
    std::cout<<"test actor model "<<std::this_thread::get_id()<<std::endl<<std::flush;
    Thread<A> thread_a("test_a");
    std::this_thread::sleep_for(0.2s);
    Thread<B> thread_b("test_b");
//    thread_b.object.do_calc("aaa",1);  // can not get object from Thread

    auto a_ObjectRef = thread_a.actor();
    auto b_ObjectRef = thread_b.actor();

    b_ObjectRef.invoke(&B::do_calc,"msg to B",a_ObjectRef);

    std::cout<<"--- Done"<<std::endl<<std::flush;
    return 0;
}

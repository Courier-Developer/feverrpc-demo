#include "msgpack.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <vector>

typedef std::string _rpc_data_t;

template <typename T> class Serializable {
  private:
    /* data */
  public:
    Serializable(T _data);
    _rpc_data_t serialize() = 0;
};

// not a typesafe method
typedef void (*voidFunctionType)(void);

class FeverRPC {
  private:
    //  强枚举类型
    enum class rpc_role { RPC_CLINT, RPC_SERVER };
    enum class rpc_ret_code : unsigned int {
        RPC_RET_SUCCESS = 0,
        RPC_RET_FUNCTION_NOT_BIND,
        RPC_RET_RECV_TIMEOUT,
        RPC_RET_UNRETURNED,
    };

    rpc_role _rpc_role;
    rpc_ret_code _rpc_ret_code;
    std::map<const char *, voidFunctionType> call_map;

  public:
    void as_clint() {
        _rpc_role = rpc_role::RPC_CLINT;
        _rpc_ret_code = rpc_ret_code::RPC_RET_UNRETURNED;
    };
    void as_server();

    template <typename T, typename... Args>
    void bind(const char *func_name, T (*func)(Args...)) {
        call_map[func_name] = (voidFunctionType)func;
    };

    // template <typename... Args>
    // auto package_args(Args... args){
    //   return std::make_tuple(args...);
    // }
    template <typename... Args> auto wrapper(Args... args) {
        return std::make_tuple(args...);
    }

    template <typename T, typename... Args>
    T call(const char *func_name, Args... args) {
        // call a function, called by client.
        // wrapper all the args to stream via buffer.
        auto t = wrapper(args...);
        auto buffer = std::stringstream{};
        buffer.seekg(0);
        msgpack::pack(buffer, t);
        std::string tuple_as_string = buffer.str();
        // print the buffer
        std::cout << tuple_as_string << std::endl;
        // the code below run on serve, the server receive the stream.
        msgpack::object_handle oh =
            msgpack::unpack(tuple_as_string.data(), tuple_as_string.size());
        msgpack::object deserialized = oh.get();
        std::cout << deserialized << std::endl << "==========";
        std::tuple<Args...> dst;
        // unpack stream to arguments
        deserialized.convert(dst);
        // get the function
        voidFunctionType func = call_map[func_name];
        // try to apply function with the argument
        T (*f)(Args...) = ((T(*)(Args...))func);
        return apply(dst, f);
    };
    template <class Tuple, class F, size_t... Is>
    constexpr auto apply_impl(Tuple t, F f, std::index_sequence<Is...>) {
        return f(std::get<Is>(t)...);
    }

    template <class Tuple, class F> constexpr auto apply(Tuple t, F f) {
        return apply_impl(t, f,
                          std::make_index_sequence<std::tuple_size<Tuple>{}>{});
    }
};

void add(int a, int b) { a + b; }

std::string adder(std::string a) {
    std::string ret = a + a;
    return ret;
}

int addee(std::vector<int> va, std::vector<int> vb, int c) {
    int ret = 0;
    for (auto v : va) {
        ret += v * v * c;
    }
    for (auto v : vb) {
        ret += v * v / c;
    }
    return ret;
}

template <typename... Args> struct index_sequence {};

int main() {
    using std::cout;
    using std::endl;
    FeverRPC rpc;
    rpc.as_clint();
    rpc.bind("add", add);
    rpc.bind("test", adder);
    rpc.call<void>("add", 1, 2);
    std::string a = "hello";
    std::string b = ", world! ";

    cout << rpc.call<std::string>("test", a) << endl;
    std::vector<int> va = {1, 2, 3, 4, 5};
    std::vector<int> vb = {2, 4, 6, 8, 10};
    rpc.bind("addee", addee);
    cout << rpc.call<int>("addee", va, vb, 2) << endl;
}
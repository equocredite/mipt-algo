#include <iostream>
#include <type_traits>

template<int...>
struct GetXor;

template<>
struct GetXor<> {
    static constexpr int value = 0;
};

template<int head, int... tail>
struct GetXor<head, tail...> {
    static constexpr int value = head ^ GetXor<tail...>::value;
};

template<int x>
struct Id {
    static constexpr int value = x;
};

template<int, int>
struct LeftmostBit;

template<int cur>
struct LeftmostBit<0, cur> {
    static constexpr int value = 0;
};

template<int num, int cur = 1>
struct LeftmostBit {
    static constexpr int value =
            std::conditional<num == cur,
                             Id<num>,
                             LeftmostBit<(num | cur) ^ cur, cur << 1>
                            >::type::value;
};

template<int...>
struct FindMove;

template<int head, int... nums>
struct FindMove<1, 0, 0, head, nums...> {
    static constexpr int who = 1 + 1,
                         whence = 0,
                         how = 0;
};

template<int pile_id, int bit, int sum>
struct FindMove<pile_id, bit, sum> {
    static constexpr int who = 0,
                         whence = 0,
                         how = 0;
};

template<int pile_id, int bit, int sum, int head, int... tail>
struct FindMove<pile_id, bit, sum, head, tail...> {
    static constexpr int who = 1;
    typedef FindMove<pile_id + 1, bit, sum, tail...> continue_search;
    static constexpr int whence = std::conditional<(head & bit) != 0,
                                                   Id<pile_id>,
                                                   Id<continue_search::whence>
                                                  >::type::value;
    static constexpr int how = std::conditional<(head & bit) != 0,
                                                Id<head - (head ^ sum)>,
                                                Id<continue_search::how>
                                               >::type::value;
};

template<int... nums>
struct AhalaiMahalai {
    static constexpr int sum = GetXor<nums...>::value;
    static constexpr int bit = LeftmostBit<sum>::value;

    typedef FindMove<1, bit, sum, nums...> move;

    static constexpr int who = move::who,
                         whence = move::whence,
                         how = move::how;
};

int main() {
    typedef AhalaiMahalai<1, 3, 7> A;
    std::cout << A::who << std::endl 
    		  << A::whence << std::endl 
              << A::how;
}

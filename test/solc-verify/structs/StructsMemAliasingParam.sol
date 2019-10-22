pragma solidity >=0.5.0;
pragma experimental ABIEncoderV2;

contract StructsMemAliasingParam {
    struct S { int x; }

    function f(S memory s1, S memory s2) public pure {
        s1.x = 1;
        s2.x = 1;
        S memory sm = S(2);
        assert(s1.x == 1);
        assert(s2.x == 1);
        assert(sm.x == 2);
        s1.x = 3;
        assert(s1.x == 3);
        assert(sm.x == 2);
        assert(s2.x == 1); // Can fail because s1 and s2 can alias
    }
}
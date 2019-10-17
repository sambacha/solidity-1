pragma solidity >=0.5.0;

contract Test {
    struct S { int x; }
    S[] s;
    function test() public {
        require(s.length > 0);
        s[0].x = 1;
        S[] storage sl = s;
        assert(sl[0].x == 1);
    }
}
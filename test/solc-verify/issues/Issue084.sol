pragma solidity >=0.5.0;

contract Issue084 {
    struct S {
        uint x;
    }

    S s;

    function() external payable {
        assert(s.x >= 0);
    }
}
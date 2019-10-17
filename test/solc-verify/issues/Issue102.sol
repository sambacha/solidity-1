pragma solidity>=0.5.0;

contract Issue102 {
    struct S {
        int x;
        bool b;
    }

    function() external payable {
        S memory s; // Allocate new and set default values
        assert(s.x == 0);
        assert(s.b == false);
    }
}
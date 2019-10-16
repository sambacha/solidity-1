pragma solidity>=0.5.0;

contract DeleteValType {
    int x;
    bool b;
    address a;

    constructor() public {
        assert(x == 0);
        assert(!b);
        assert(a == address(0x0));

        x = 1;
        b = true;
        a = address(0x123);
        assert(x == 1);
        assert(b);
        assert(a == address(0x123));

        delete x;
        delete b;
        delete a;
        assert(x == 0);
        assert(!b);
        assert(a == address(0x0));
    }

    function() external payable {
    }

}
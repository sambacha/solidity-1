// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Swap {
    uint a;
    uint b;

    receive() external payable {
        uint x = 1;
        uint y = 2;
        (x, y) = (y, x);
        assert(x == 2);
        assert(y == 1);

        a = 1;
        b = 2;
        (a, b) = (b, a);
        assert(a == 2);
        assert(b == 1);
    }
}

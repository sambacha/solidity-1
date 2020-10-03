// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract DeleteValType {
    int x;
    bool b;
    address a;

    constructor() {
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

    receive() external payable {
    }

}

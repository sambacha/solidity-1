// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract EvalOrder {
    uint[] x;

    constructor() {
        x.push(3);
        x.push(0);
        x.push(2);
        uint i = 0;
        x[i++] = ++i;
        assert(x[0] == 3);
        assert(x[1] == 1);
        assert(x[2] == 2);
    }

    receive() external payable {
    }
}

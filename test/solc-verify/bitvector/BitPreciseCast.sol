// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract BitPreciseCast {

    receive() external payable {
        uint8 x = 255;
        assert(int8(x) == -1);
        uint16 y = 257;
        assert(uint8(y) == 1);
        int8 z = -3;
        assert(uint8(z) == 253);
        uint8 t = 1;
        uint16 t2 = 1;
        assert(t == t2);
    }
}

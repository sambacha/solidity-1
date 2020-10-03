// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract IntLiterals {
    receive() external payable {
        assert(123_456 == 123456);
        assert(12_345 == 12345);
        assert(1e5 == 100000);
        assert(1_5e2 == 1500);
    }
}

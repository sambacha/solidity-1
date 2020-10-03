// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

// To be used together with Import2.sol
contract A {
    /** @notice postcondition x == y */
    function f(uint x) public pure returns (uint y) {
        return x;
    }
}

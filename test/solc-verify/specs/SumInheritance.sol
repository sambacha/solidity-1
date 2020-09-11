// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/// @notice invariant __verifier_sum_int(map) == total
contract Base {
    mapping(address=>int) map;
    int total;

    function add(int x) public {
        map[msg.sender] += x;
        total += x;
    }
}

/// @notice invariant __verifier_sum_int(map) == total
contract Derived is Base {
    function sub(int x) public {
        map[msg.sender] -= x;
        total -= x;
    }
}
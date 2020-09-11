// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/// @notice invariant map[0] == 0
contract DefaultBvMapping {

    mapping(int16=>int32) map;

    constructor() public {
        assert(map[0] == 0);
    }

    receive() external payable {
        assert(map[0] == 0);
    }
}

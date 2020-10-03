// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue081 {
    bool marked;

    /// @notice modifies marked if !marked
    function mark() public {
        require(!marked);
        marked = true;
    }
}

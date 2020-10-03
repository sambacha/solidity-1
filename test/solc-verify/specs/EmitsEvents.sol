// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract EmitsEvents {

    event Increased();
    uint x;

    /// @notice emits Increased
    function increase() public {
        x++;
        emit Increased();
    }

    /// @notice emits Increased
    /// @notice emits Increased
    /// @notice emits Increased
    function increaseWarning() public {
        x++;
        emit Increased();
    }
}

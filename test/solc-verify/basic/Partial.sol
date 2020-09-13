// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Partial {
    uint x;
    uint y;

    /// @notice modifies x
    /// @notice postcondition x == __verifier_old_uint(x) + 1
    function unsupported() internal {
        assembly {
            let t := sload(x.slot)
            t := add(t, 1)
            sstore(x.slot, t)
        }
    }

    // Last postcondition should fail
    /// @notice modifies x
    /// @notice modifies y
    /// @notice modifies address(this).balance
    /// @notice postcondition x != y
    receive() external payable {
        x = 1;
        y = 2;
        unsupported();
        assert(x == y); // Should hold
    }
}

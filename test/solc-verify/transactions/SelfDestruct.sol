// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/**
 * @notice invariant address(this).balance == 0
 */
contract NonPayable {
    receive() external payable {
        require(false);
    }
}

contract ToBeKilled {
    NonPayable public p;

    /// @notice postcondition p == np
    constructor(NonPayable np) {
        p = np;
    }

    /// @notice postcondition address(p).balance >= msg.value
    receive() external payable {
        selfdestruct(address(p));
    }
}

/// @notice invariant p == tbk.p()
contract SelfDestruct {

    NonPayable p;
    ToBeKilled tbk;

    constructor() {
        p = new NonPayable();
        tbk = new ToBeKilled(p);
    }

    receive() external payable {
        require(msg.value > 0);
        (bool b,) = address(tbk).call{value: msg.value}("");
        require(b);
        assert(address(p).balance > 0);
    }

}

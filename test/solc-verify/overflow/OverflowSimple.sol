// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/**
 * @notice invariant x == y
 */
contract OverflowSimple {

    uint8 x;
    uint8 y;

    function inc_overflow() public {
        x = x + 1;
        y = y + 1;
    } // Overflow at end

    function inc_no_overflow() public {
        x = x + 1;
        require(x > y);
        y = y + 1;
        // No require needed for y, the one for x and the invariant is sufficient
    }

    function inc_assignadd_overflow() public {
        x += 1;
        y += 1;
    }  // Overflow at end

    function inc_assignadd_no_overflow() public {
        x += 1;
        require(x > y);
        y += 1;
        // No require needed for y, the one for x and the invariant is sufficient
    }

    function sideeffect_overflow() public {
        y = ++x;
    }

    function sideeffect_no_overflow() public {
        ++x;
        require(x > y);
        y++;
    }

    function inc_call_overflow() public {
        x = x + 1;
        nothing(); // Overflow before call
        require(x > y);
        y = y + 1;
    }
    function inc_call_no_overflow() public {
        x = x + 1;
        require(x > y);
        nothing(); // OK
        y = y + 1;
    }

    function nothing() private {}

    function loop_no_overflow() public {
        /**
         * @notice {contractInvariants}
         */
        for (uint8 i = 0; i < 10; i = i + 1) {
            x = x + 1;
            require(x > y);  // OK
            y = y + 1;
        }
    }

    function loop_overflow() public {
        /**
         * @notice {contractInvariants}
         */
        for (uint8 i = 0; i < 10; i = i + 1) {
            x = x + 1;
            y = y + 1;
        } // Overflow in loop
    }

    function whileloop_no_overflow() public {
        uint8 i = 0;
        /**
         * @notice {contractInvariants}
         */
        while (i < 10) {
            x = x + 1;
            require(x > y);  // OK
            y = y + 1;
            i = i + 1;
        }
    }

    function whileloop_overflow() public {
        uint8 i = 0;
        /**
         * @notice {contractInvariants}
         */
        while (i < 10) {
            x = x + 1;
            y = y + 1;
            i = i + 1;
        } // Overflow in loop
    }

     function arrayLength(uint[] memory arr) public pure {
        for (uint i = 0; i < arr.length; i++) {
            // No overflow for i++
        }
    }
}

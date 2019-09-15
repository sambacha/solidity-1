pragma solidity >=0.5.0;

/// @notice invariant x == y
contract C {
    int x;
    int y;

    function incr() public {
        x++;
        y++;
    }

    function test() public {
        x = 0;
        y = 0;
        // Invariant holds before external call
        (bool ok, ) = msg.sender.call("");
        assert(x == y); // It should also hold afterwards
        if (ok){
            assert(x == 0); // However this should fail because variables could be modified
        } else {
            assert(x == 0); // This should hold because variables were not modified
        }
    }
}
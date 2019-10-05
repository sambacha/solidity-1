pragma solidity>=0.5.0;

contract Issue098 {
    struct S {
        int balance;
    }

    S s;

    /// @notice modifies s.balance
    function f() public {
        s.balance = 5;
    }
}
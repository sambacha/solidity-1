pragma solidity >=0.5.0;

contract Issue081 {
    bool marked;

    /// @notice modifies marked if !marked
    function mark() public {
        require(!marked);
        marked = true;
    }
}

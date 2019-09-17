pragma solidity >=0.5.0;

/// @notice invariant map[0] == 0
contract DefaultBvMapping {

    mapping(int16=>int32) map;

    constructor() public {
        assert(map[0] == 0);
    }

    function() external payable {
        assert(map[0] == 0);
    }
}
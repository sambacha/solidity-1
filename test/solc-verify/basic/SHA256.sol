pragma solidity >=0.5.0;

contract SHA256 {

  /// @notice postcondition ok
  function hashTwice(bytes memory input) public pure returns (bool ok) {
    bytes32 h1 = sha256(input);
    bytes32 h2 = sha256(input);
    ok = (h1 == h2);
  }

  function() external payable {
    assert(hashTwice("test"));
  }
}

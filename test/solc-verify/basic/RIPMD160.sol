pragma solidity >=0.5.0;

contract RIPMD160 {

  /// @notice postcondition ok
  function hashTwice(bytes memory input) public pure returns (bool ok) {
    bytes20 h1 = ripemd160(input);
    bytes20 h2 = ripemd160(input);
    ok = (h1 == h2);
  }

  function() external payable {
    assert(hashTwice("test"));
  }
}

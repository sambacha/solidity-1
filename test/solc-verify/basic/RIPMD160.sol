// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract RIPMD160 {

  /// @notice postcondition ok
  function hashTwice(bytes memory input) public pure returns (bool ok) {
    bytes20 h1 = ripemd160(input);
    bytes20 h2 = ripemd160(input);
    ok = (h1 == h2);
  }

  receive() external payable {
    assert(hashTwice("test"));
  }
}

// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Keccak256 {

  /// @notice postcondition ok
  function hashTwice(bytes memory input) public pure returns (bool ok) {
    bytes32 h1 = keccak256(input);
    bytes32 h2 = keccak256(input);
    ok = (h1 == h2);
  }

  receive() external payable {
    assert(hashTwice("test"));
  }
}

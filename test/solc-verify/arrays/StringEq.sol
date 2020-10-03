// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract StringEq {

  struct Name {
    bool isSet;
    string name;
  }

  mapping(address=>Name) names;

  /// @notice precondition !names[msg.sender].isSet
  /// @notice postcondition names[msg.sender].isSet && __verifier_eq(names[msg.sender].name, name)
  event NewName(address a, string name);

  /// @notice emits NewName
  /// @notice postcondition names[msg.sender].isSet
  /// @notice postcondition __verifier_eq(names[msg.sender].name, name)
  function add(string memory name) public {
    require(!names[msg.sender].isSet);
    names[msg.sender].name = name;
    names[msg.sender].isSet = true;
    emit NewName(msg.sender, name);
  }

}

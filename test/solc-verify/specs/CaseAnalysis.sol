// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract CaseAnalysis {

  struct S {
    bool initialized;
    int value;
  }

  mapping(address=>S) data;

  /// @notice specification
  /// [
  ///   case data[msg.sender].initialized && value > data[msg.sender].value:
  ///     data[msg.sender].initialized && data[msg.sender].value == value;
  ///   case !data[msg.sender].initialized:
  ///     data[msg.sender].initialized && data[msg.sender].value == value;
  /// ]
  function add(int value) public {
    S storage s = data[msg.sender];
    if (s.initialized) {
      if (value > s.value) {
        s.value = value;
      }
    } else {
      s.initialized = true;
      s.value = value;
    }
  }

  mapping(address=>uint) balances;

  /// @notice specification
  /// [
  ///    case msg.sender != to && balances[msg.sender] >= value:
  ///      balances[msg.sender] == __verifier_old_uint(balances[msg.sender]) - value &&
  ///      balances[to] == __verifier_old_uint(balances[to]) + value;
  /// ]
  function transfer(address to, uint value) public {
    require (balances[msg.sender] >= value);
    balances[msg.sender] -= value;
    balances[to] += value;
  }
}


/// @notice invariant property(members) (i, j) (i == j || members[i] != members[j])
contract Members {

  address[] members;

  /// @notice specification [
  ///   case property(members) (i) (members[i] != member):
  ///      (members.length == __verifier_old_uint(members.length) + 1) &&
  ///      (members[members.length-1] == member);
  /// ]
  function addMember(address member) public {
    /// @notice invariant property(members) (j) (j >= i || members[j] != member)
    for (uint i = 0; i < members.length; ++ i) {
      if (members[i] == member) {
        return;
      }
    }
    members.push(member);
  }

  /// @notice specification [
  ///   case exists(uint i) (0 <= i && i < members.length && members[i] == member):
  ///      members.length == __verifier_old_uint(members.length) - 1;
  /// ]
  function removeMember(address member) public {
    /// @notice invariant property(members) (j) (j >= i || members[j] != member)
    for (uint i = 0; i < members.length; ++ i) {
      if (members[i] == member) {
        members[i] = members[members.length - 1];
        members.pop();
        return;
      }
    }
  }
}

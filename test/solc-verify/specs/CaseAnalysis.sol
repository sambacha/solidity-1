pragma solidity>=0.5.0;

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
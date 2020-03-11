pragma solidity >=0.5.0;

contract C {

  struct Data {
    bool initialized;
    uint value;
  }

  mapping(uint=>Data) data;

  /// @notice tracks-changes-in data
  /// @notice precondition !data[i].initialized
  /// @notice postcondition data[i].initialized && data[i].value == value
  event new_entry(uint i, uint value);

  /// @notice tracks-changes-in data
  /// @notice precondition data[i].initialized
  /// @notice postcondition data[i].initialized && data[i].value == value
  event updated_entry(uint i, uint value);

  /// @notice emits new_entry
  /// @notice emits updated_entry
  function test_new_entry() public {
    require(!data[0].initialized);
    data[0].initialized = true;
    data[0].value = 0;
    emit new_entry(0, 0);
    data[0].value = 1;
    emit updated_entry(0, 1);
  }

  /// @notice emits updated_entry
  function test_updated_entry() public {
    require(data[0].initialized);
    data[0].value = 1;
    emit updated_entry(0, 1);
  }

}

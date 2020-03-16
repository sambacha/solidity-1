pragma solidity >=0.5.0;

contract C {

  struct Data {
    bool initialized;
    uint value;
  }

  mapping(uint=>Data) data;

  address lastUpdate;

  /// @notice tracks-changes-in data
  /// @notice tracks-changes-in lastUpdate
  /// @notice precondition !data[i].initialized
  /// @notice postcondition data[i].initialized && data[i].value == value
  /// @notice postcondition lastUpdate == msg.sender
  event new_entry(uint i, uint value);

  /// @notice tracks-changes-in data
  /// @notice tracks-changes-in lastUpdate
  /// @notice precondition data[i].initialized
  /// @notice postcondition data[i].initialized && data[i].value == value
  /// @notice postcondition lastUpdate == msg.sender
  event updated_entry(uint i, uint value);

  /// @notice emits new_entry
  /// @notice emits updated_entry
  function test_new_entry() public {
    require(!data[0].initialized);
    data[0].initialized = true;
    data[0].value = 0;
    lastUpdate = msg.sender;
    emit new_entry(0, 0);
    data[0].value = 1;
    lastUpdate = msg.sender;
    emit updated_entry(0, 1);
  }

  /// @notice emits updated_entry
  function test_updated_entry() public {
    require(data[0].initialized);
    data[0].value = 1;
    lastUpdate = msg.sender;
    if (data[1].value == 3) {
      emit updated_entry(0, 1);
    }
  }

  int a;
  int b;

  /// @notice tracks-changes-in a
  /// @notice tracks-changes-in b
  /// @notice precondition a < b
  /// @notice postcndition a < b
  event a_b_changed();

  /// @notice emits a_b_changed
  function test_a_b_changed() public {
    require(a < b);
    b ++;
    emit a_b_changed();
    a ++;
    emit a_b_changed();
  }

  event no_data_changed();

  /// @notice emits no_data_changed
  function emit_no_data_changed() public { emit no_data_changed(); }

}

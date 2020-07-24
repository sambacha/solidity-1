pragma solidity>=0.5.0;

/// @notice invariant property(unique) (i, j) (i == j || unique[i] != unique[j])
/// @notice invariant property(sorted) (i, j) (i > j || sorted[i] <= sorted[j])
/// @notice invariant property(entryList) (i) entryMap[entryList[i]].initialized
/// @notice invariant property(entryList) (i, j) (i == j || entryList[i] != entryList[j])
/// @notice invariant forall (address a) exists (uint j) !entryMap[a].initialized || (0 <= j && j < entryList.length && entryList[j] == a)
contract ArrayProperties {

  // an array of unique elements
  int[] unique;

  /// @notice modifies unique
  function addUnique(int x) public {
    /// @notice invariant 0 <= i && i <= unique.length
    /// @notice invariant property(unique) (j) (j >= i) || (unique[j] != x)
    for (uint i = 0; i < unique.length; ++ i) {
      if (unique[i] == x) return;
    }
    unique.push(x);
  }

  // an array of sorted elements
  uint[] sorted;

  /// @notice modifies sorted
  function addToSorted() public {
    if (sorted.length == 0) {
      sorted.push(0);
    } else {
      uint last = sorted[sorted.length-1];
      sorted.push(last + 1);
    }
  }

  struct Entry {
    bool initialized;
    int value;
  }

  address[] entryList;
  mapping(address=>Entry) entryMap;

  /// @notice modifies entryMap[msg.sender]
  /// @notice modifies entryList
  /// @notice postcondition entryMap[msg.sender].initialized && entryMap[msg.sender].value == value
  function setValue(int value) public {
    Entry storage e = entryMap[msg.sender];
    if (e.initialized) {
      e.value = value;
    } else {
      e.initialized = true;
      e.value = value;
      entryList.push(msg.sender);
    }
  }

}

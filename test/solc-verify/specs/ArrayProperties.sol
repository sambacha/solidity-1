pragma solidity>=0.5.0;

/// @notice invariant property(unique) (i, j) (i == j || unique[i] != unique[j])
/// @notice invariant property(sorted) (i, j) (i > j || sorted[i] <= sorted[j])
/// @notice invariant property(entryList) (a) (entryMap[entryList[a]].initialized)
contract ArrayProperties {

  // an array of unique elements
  int[] unique;

  // an array of sorted elements
  uint[] sorted;

  struct Entry {
    bool initialized;
    int value;
  }

  address[] entryList;
  mapping(address=>Entry) entryMap;

}

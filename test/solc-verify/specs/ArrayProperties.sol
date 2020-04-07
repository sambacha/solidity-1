pragma solidity>=0.5.0;

/// @notice invariant property(unique) (i, j) (i == j || unique[i] != unique[j])
/// @notice invariant property(sorted) (i, j) (i > j || sorted[i] <= sorted[j])
contract ArrayProperties {

  // an array of unique elements
  int[] unique;

  // an array of sorted elements
  uint[] sorted;

}

pragma solidity >=0.5.0;

contract ItemListing
{
    enum StateType { ItemAvailable, ItemSold }

    StateType public State;

    address public Seller;
    address public InstanceBuyer;
    address public ParentContract;
    string public ItemName;
    int public ItemPrice;
    address public PartyA;
    address public PartyB;


    constructor(string memory itemName, int itemPrice, address seller, address parentContractAddress, address partyA, address partyB) public {
        Seller = seller;
        ParentContract = parentContractAddress;
        ItemName = itemName;
        ItemPrice = itemPrice;

        PartyA = partyA;
        PartyB = partyB;

        State = StateType.ItemAvailable;
    }

    function BuyItem() public
    {
        InstanceBuyer = msg.sender;

        // ensure that the buyer is not the seller
        if (Seller == InstanceBuyer) {
            revert();
        }

        Bazaar bazaar = Bazaar(ParentContract);

        // check Buyer's balance
        if (!bazaar.HasBalance(InstanceBuyer, ItemPrice)) {
            revert();
        }

        // indicate item bought by updating seller and buyer balances
        bazaar.UpdateBalance(Seller, InstanceBuyer, ItemPrice);

        State = StateType.ItemSold;
    }
}

contract Bazaar
{
    enum StateType { PartyProvisioned, ItemListed, CurrentSaleFinalized}

    StateType public State;

    address public InstancePartyA;
    int public PartyABalance;

    address public InstancePartyB;
    int public PartyBBalance;

    address public InstanceBazaarMaintainer;
    address public CurrentSeller;

    string public ItemName;
    int public ItemPrice;

    ItemListing currentItemListing;
    address public CurrentContractAddress;

    constructor(address partyA, int balanceA, address partyB, int balanceB) public {
        InstanceBazaarMaintainer = msg.sender;

        // ensure the two parties are different
        if (partyA == partyB) {
            revert();
        }

        InstancePartyA = partyA;
        PartyABalance = balanceA;

        InstancePartyB = partyB;
        PartyBBalance = balanceB;

        CurrentContractAddress = address(this);

        State = StateType.PartyProvisioned;
    }

    function HasBalance(address buyer, int itemPrice) public view returns (bool) {
        if (buyer == InstancePartyA) {
            return (PartyABalance >= itemPrice);
        }

        if (buyer == InstancePartyB) {
            return (PartyBBalance >= itemPrice);
        }

        return false;
    }

    function UpdateBalance(address sellerParty, address buyerParty, int itemPrice) public {
        ChangeBalance(sellerParty, itemPrice);
        ChangeBalance(buyerParty, -itemPrice);

        State = StateType.CurrentSaleFinalized;
    }

    function ChangeBalance(address party, int balance) public {
        if (party == InstancePartyA) {
            PartyABalance += balance;
        }

        if (party == InstancePartyB) {
            PartyBBalance += balance;
        }
    }

    function ListItem(string memory itemName, int itemPrice) public
    {
        CurrentSeller = msg.sender;

        currentItemListing = new ItemListing(itemName, itemPrice, CurrentSeller, CurrentContractAddress, InstancePartyA, InstancePartyB);

        State = StateType.ItemListed;
    }
}

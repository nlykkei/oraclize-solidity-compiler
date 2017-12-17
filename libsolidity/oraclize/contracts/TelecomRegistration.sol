/** 
 *  @file    TelecomRegistration.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Telecom registration. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for restrictive telecom registration using "kds" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract TelecomRegistration is OraclizeSolidity {

  event LogEvent(string msg);
  event LogEvent(string msg, uint region, address provider);
  event LogEvent(string msg, uint region, address provider, address buyer);
  event LogFunds(string msg, uint funds, address sender);
  event LogFunds(string msg, uint funds, address sender, address receiver);

  /*
   * List of registrered EOA's for telecom providers
   */

  uint constant NUM_REGIONS = 10;

  address creator;

  struct TelecomRegion {
    address provider;
    Sale sale;
    bool[10] provider_links; // up := 1, down := 0
  }

  struct Sale {
    bool forsale;
    uint price;
  }

  struct PendingTrade {
    bool active;
    address buyer;
    bool[10] buyer_links;
    uint8 region;
  }

  PendingTrade pending_trade;
  TelecomRegion[10] telecom_regions;   

  function TelecomRegistration() public payable {
    creator = msg.sender;
    for (uint i = 0; i < NUM_REGIONS; ++i) {
      telecom_regions[i].provider = msg.sender;
      telecom_regions[i].sale.forsale = true;
      telecom_regions[i].sale.price = 1000000000000000000; // 1 ether
    }
    for (uint j = 0; j < NUM_REGIONS; ++j) {
      telecom_regions[0].provider_links[j] = true; // Connect region 0 to all other regions
    }
  }

  function tradeTelecom(uint8 region, bool[10] buyer_links) public payable {
    uint i; uint j;

    /*
     * Check sender is valid EOA for telecom provider
     */

    // Check no pending trade
    if (pending_trade.active) {
      LogEvent("Telecom trade: Fail: Pending trade already active");
      return;
    }

    // Telecom provider implicitly links to itself
    buyer_links[region] = false;

    // Telecom carier must provide at least two external links
    uint num_links = 0;
    for (i = 0; i < NUM_REGIONS; ++i) {
      if (buyer_links[i]) ++num_links;
    }

    if (num_links < 2) {
      LogEvent("Telecom trade: Must provide at least two links");
      return;
    }

    if (telecom_regions[region].sale.forsale) {
      if (msg.value < telecom_regions[region].sale.price) { // Hold funds until trade completion
        LogFunds("Telecom trade: Insufficient funds", msg.value, msg.sender);
        msg.sender.send(msg.value);
        return;
      }

      // Activate pending trade
      pending_trade.active = true;
      pending_trade.buyer = msg.sender;
      pending_trade.buyer_links = buyer_links;
      pending_trade.region = region;

      uint[] memory link_map = new uint[](NUM_REGIONS ** 2);
      for (i = 0; i < NUM_REGIONS - 1; ++i) {
        link_map[i * NUM_REGIONS + i] = 1;
        for (j = i + 1; j < NUM_REGIONS; ++j) { // (0,1),..,(0,n-1),(1,2),..,(1,n-1),..,(n-2,n-1)
          uint link;
          if (i == region) {
            link = buyer_links[j] || telecom_regions[j].provider_links[i] ? 1 : 0;
          } else if (j == region) {
            link = telecom_regions[i].provider_links[j] || buyer_links[i] ? 1 : 0;
          } else {
            link = telecom_regions[i].provider_links[j] || telecom_regions[j].provider_links[i] ? 1 : 0;
          }
          link_map[i * NUM_REGIONS + j] = link_map[j * NUM_REGIONS + i] = link;
        }
      }
      link_map[(NUM_REGIONS - 1) * NUM_REGIONS + (NUM_REGIONS - 1)] = 1;

      LogEvent("Telecom trade: Processing trade", pending_trade.region, telecom_regions[pending_trade.region].provider, pending_trade.buyer);
      processTrade(link_map, 3);
    } 
    else {
      LogEvent("Telecom trade: The region must be for sale", region, telecom_regions[region].provider);
    }
  }

  function forSale(uint8 region, uint price) public {
    if (telecom_regions[region].provider != msg.sender) {
      LogEvent("For sale: Only provider of region can set it for sale", region, telecom_regions[region].provider);
      return;
    }

    LogEvent("For sale: The region is now for sale", region, telecom_regions[region].provider);
    telecom_regions[region].sale.forsale = true;
    telecom_regions[region].sale.price = price; // wei
  } 

  function forSale(uint8 region) public constant returns(bool) {
    if (telecom_regions[region].sale.forsale) {
      LogFunds("For sale: The region is for sale", telecom_regions[region].sale.price, telecom_regions[region].provider);
    } else {
      LogEvent("For sale: The region is not for sale", region, telecom_regions[region].provider);
    }

    return telecom_regions[region].sale.forsale;
  }

  function processTrade(uint[] link_map, uint k) private {
    oracleQuery("kds", link_map, k, finalize, true);	
  }

  function finalize(string result) private {
    if (bytes(result).length == 0) {
      LogEvent("Trading region: Fail (criteria)", pending_trade.region, telecom_regions[pending_trade.region].provider);
      return;
    }

    LogEvent("Trading region: Success", pending_trade.region, telecom_regions[pending_trade.region].provider, pending_trade.buyer);

    LogFunds("Trading region: Sending funds to seller", telecom_regions[pending_trade.region].sale.price, pending_trade.buyer, telecom_regions[pending_trade.region].provider);
    telecom_regions[pending_trade.region].provider.send(telecom_regions[pending_trade.region].sale.price);

    telecom_regions[pending_trade.region].provider = pending_trade.buyer;
    LogEvent("Trading Region: New region provider",  pending_trade.region, telecom_regions[pending_trade.region].provider);
    delete telecom_regions[pending_trade.region].sale;
    telecom_regions[pending_trade.region].provider_links = pending_trade.buyer_links;
    delete pending_trade;
  }

  function getRegionProvider(uint region) public constant returns(address) {
    LogEvent("Region provider:", region, telecom_regions[region].provider);
    return telecom_regions[region].provider;
  }

  function destruct() public {
    if (msg.sender == creator) {
        LogEvent("Destruct: Balance to creator");
        selfdestruct(creator);
    }
  }
}
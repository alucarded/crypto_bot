import unittest
from order_book import OrderBook


class TestOrderBook(unittest.TestCase):
  def test_basic(self):
    ob = OrderBook(100)
    bid_updates = [(1.1, 100), (1.19, 200)]
    ask_updates = [(1.3, 100), (1.2, 200), (1.25, 123), (1.3, 0)]
    for bid in bid_updates:
      if bid[1] == 0:
        ob.delete_bid(bid[0])
      else:
        ob.upsert_bid(bid[0], bid[1])
    for ask in ask_updates:
      if ask[1] == 0:
        ob.delete_ask(ask[0])
      else:
        ob.upsert_ask(ask[0], ask[1])
    ob_bids = ob.get_bids()
    ob_asks = ob.get_asks()
    self.assertListEqual([(1.1, 100), (1.19, 200)], ob_bids)
    self.assertListEqual([(1.25, 123), (1.2, 200)], ob_asks)

if __name__ == '__main__':
    unittest.main()
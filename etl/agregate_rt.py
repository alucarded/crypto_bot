from order_book import OrderBook

import argparse
from pymongo import MongoClient
import json


def handle_trade_ticker(doc):
  market_sell_vol = 0
  market_buy_vol = 0
  trade_weighted_sell_price = 0
  trade_weighted_buy_price = 0
  for ticker in doc["tickers"]:
    if ticker["is_market_maker"]:
      market_sell_vol += ticker["qty"]
      trade_weighted_sell_price += ticker["qty"]*ticker["price"]
    else:
      market_buy_vol += ticker["qty"]
      trade_weighted_buy_price += ticker["qty"]*ticker["price"]
  trade_weighted_sell_price /= market_sell_vol
  trade_weighted_buy_price /= market_buy_vol
  del doc["tickers"]
  doc["market_sell_vol"] = market_sell_vol
  doc["market_buy_vol"] = market_buy_vol
  doc["weighted_sell_price"] = trade_weighted_sell_price
  doc["weighted_buy_price"] = trade_weighted_buy_price
  return doc



def handle_book_ticker(doc):
  ob = None
  hb = None
  lb = None
  cb = None
  oa = None
  ha = None
  la = None
  ca = None
  for ticker in doc["tickers"]:
    if not ob:
      ob = ticker["bid"]
      hb = ticker["bid"]
      lb = ticker["bid"]
    if not oa:
      oa = ticker["ask"]
      ha = ticker["ask"]
      la = ticker["ask"]
    cb = ticker["bid"]
    ca = ticker["ask"]
    hb = max(hb, ticker["bid"])
    ha = max(ha, ticker["ask"])
    lb = min(lb, ticker["bid"])
    la = min(la, ticker["ask"])
  del doc["tickers"]
  doc["ask"] = {}
  doc["ask"]["o"] = oa
  doc["ask"]["h"] = ha
  doc["ask"]["l"] = la
  doc["ask"]["c"] = ca
  doc["bid"] = {}
  doc["bid"]["o"] = ob
  doc["bid"]["h"] = hb
  doc["bid"]["l"] = lb
  doc["bid"]["c"] = cb
  return doc



def handle_order_book_update(ob, doc):
  print(doc)
  for update in doc["updates"]:
    for bid in update["bids"]:
      price = float(bid[0])
      vol = float(bid[1])
      if vol == 0:
        ob.delete_bid(price)
      else:
        ob.upsert_bid(price, vol)
    for ask in update["asks"]:
      price = float(ask[0])
      vol = float(ask[1])
      if vol == 0:
        ob.delete_ask(price)
      else:
        ob.upsert_ask(price, vol)
  del doc["updates"]
  bids, asks = ob.group_levels(0.01)
  doc["asks"] = asks
  doc["bids"] = bids
  print(doc)
  return doc

def main():
  parser = argparse.ArgumentParser(description='Aggregate real-time crypto data into fixed periods')
  parser.add_argument('--config', help='Path to configuration file')
  args = parser.parse_args()

  f = open(args.config)
  config = json.load(f)

  client = MongoClient("mongodb://" + config["user"] + ":" + "DRt99xd4o7PMfygqotE8"
          + "@" + config["host"] + ":" + config["port"]
          + "/?authSource=" + config["authSource"])
  db = client[config["db"]]
  rt_collection = db[config["collection"]]
  book_ticker_minute_collection = db["BookTickerMinutes"]
  trade_ticker_minute_collection = db["TradeTickerMinutes"]
  ob_minute_collection = db["OrderBookMinutes"]

  ob = OrderBook(1000)
  cursor = rt_collection.find()
  for doc in cursor:
    #print(doc)
    if doc["type"] == "BOOK_TICKER":
      agg_doc = handle_book_ticker(doc)
      book_ticker_minute_collection.insert_one(agg_doc)
    elif doc["type"] == "TRADE_TICKER":
      agg_doc = handle_trade_ticker(doc)
      trade_ticker_minute_collection.insert_one(agg_doc)
    elif doc["type"] == "ORDER_BOOK":
      agg_doc = handle_order_book_update(ob, doc)
      ob_minute_collection.insert_one(agg_doc)
      #print(agg_doc)

if __name__ == "__main__":
  main()
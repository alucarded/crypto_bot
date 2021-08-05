import argparse
import pandas as pd
from pymongo import MongoClient
import json

def main():
  parser = argparse.ArgumentParser(description='Pull ticker data from Mongo, clean and prepare for training statistical model, write as csv')
  parser.add_argument('--config', help='Path to configuration file')
  parser.add_argument('--output', help='Output file name')
  args = parser.parse_args()

  f = open(args.config)
  config = json.load(f)

  client = MongoClient("mongodb://" + config["user"] + ":" + "DRt99xd4o7PMfygqotE8"
          + "@" + config["host"] + ":" + config["port"]
          + "/?authSource=" + config["authSource"])
  db = client[config["db"]]
  book_ticker_minute_collection = db["BookTickerMinutes"]
  trade_ticker_minute_collection = db["TradeTickerMinutes"]
  ob_minute_collection = db["OrderBookMinutes"]

  min_minute = 10000000000000
  max_minute = 0
  # (mid_open, mid_high, mid_low, mid_close)
  book_ticker_candles = {}
  # (open, high, low, close)
  trade_ticker_candles = {}
  # (sell_volume, buy_volume, weighted_sell_vol, weighted_buy_vol)
  trade_ticker_volumes = {}
  # (book_ask_volume_1_percent, book_bid_volume_1_percent, book_ask_volume_5_percent, book_bid_volume_5_percent)
  order_book_volumes = {}
  for doc in trade_ticker_minute_collection.find():
    current_minute = doc["minute_utc"]
    max_minute = max(current_minute, max_minute)
    min_minute = min(current_minute, min_minute)
    trade_ticker_candles[current_minute] = (doc["o"], doc["h"], doc["l"], doc["c"])
    trade_ticker_volumes[current_minute] = (doc["market_sell_vol"], doc["market_buy_vol"], doc["weighted_sell_price"], doc["weighted_buy_price"])
  for doc in book_ticker_minute_collection.find():
    current_minute = doc["minute_utc"]
    max_minute = max(current_minute, max_minute)
    min_minute = min(current_minute, min_minute)
    book_ticker_candles[current_minute] = (doc["mid"]["o"], doc["mid"]["h"], doc["mid"]["l"], doc["mid"]["c"])
  for doc in ob_minute_collection.find():
    current_minute = doc["minute_utc"]
    max_minute = max(current_minute, max_minute)
    min_minute = min(current_minute, min_minute)
    asks = doc["asks"]
    bids = doc["bids"]
    asks_len = len(asks)
    asks_sum_5 = 0
    for i in range(5):
      if i >= asks_len:
        break
      asks_sum_5 += asks[i]
    bids_len = len(bids)
    bids_sum_5 = 0
    for i in range(5):
      if i >= bids_len:
        break
      bids_sum_5 += bids[i]
    order_book_volumes[current_minute] = (asks[0], bids[0], asks_sum_5, bids_sum_5)

  # Initialize dataframe
  index = [i for i in range(min_minute, max_minute + 1)]
  columns = [ "BookOpenMid", "BookHighMid", "BookLowMid", "BookCloseMid", "TradeOpen", "TradeHigh", "TradeLow", "TradeClose", "SellVolume", "BuyVolume", "PriceWeightedSellVolume", "PriceWeightedBuyVolume", "BookAskVolume1Pctg", "BookBidVolume1Pctg", "BookAskVolume5Pctg", "BookBidVolume5Pctg" ]
  data = []
  for i in index:
    row = []
    if i in book_ticker_candles:
      row.extend(book_ticker_candles[i])
    else:
      row.extend((None, None, None, None))
    if i in trade_ticker_candles:
      row.extend(trade_ticker_candles[i])
    else:
      row.extend((None, None, None, None))
    if i in trade_ticker_volumes:
      row.extend(trade_ticker_volumes[i])
    else:
      row.extend((None, None, None, None))
    if i in order_book_volumes:
      row.extend(order_book_volumes[i])
    else:
      row.extend((None, None, None, None))
    data.append(row)

  df = pd.DataFrame(data, index = index, columns = columns)
  df.to_csv(args.output)

if __name__ == "__main__":
  main()
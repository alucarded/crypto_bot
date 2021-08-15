class OrderBook:
  def __init__(self, depth) -> None:
      self.bids = []
      self.asks = []
      self.depth = depth

  def upsert_bid(self, price, vol):
    i = 0
    found = False
    while i < len(self.bids):
      if self.bids[i][0] == price:
        self.bids[i] = (price, vol)
        found = True
        break
      elif price < self.bids[i][0]:
        break
      i += 1
    if not found:
      self.bids.insert(i, (price, vol))
    if len(self.bids) > self.depth:
      self.bids.pop(0)


  def upsert_ask(self, price, vol):
    i = 0
    found = False
    while i < len(self.asks):
      if self.asks[i][0] == price:
        self.asks[i] = (price, vol)
        found = True
        break
      elif price > self.asks[i][0]:
        break
      i += 1
    if not found:
      self.asks.insert(i, (price, vol))
    if len(self.asks) > self.depth:
      self.asks.pop(0)

  def delete_bid(self, price):
    self._delete(self.bids, price)

  def delete_ask(self, price):
    self._delete(self.asks, price)

  def _delete(self, price_levels, price):
    for i in range(len(price_levels)):
      if price_levels[i][0] == price:
        price_levels.pop(i)
        break

  def get_bids(self):
    return self.bids

  def get_asks(self):
    return self.asks

  def group_levels(self, percentage_interval):
    mid_price = (self.bids[-1][0] + self.asks[-1][0])/2
    #print(self.bids, self.asks)
    #print("Best ask: {}, best bid: {}, mid: {}", self.asks[-1][0], self.bids[-1][0], mid_price)
    interval = percentage_interval*mid_price
    # Bids
    k = -1
    groupped_bids = []
    bids_size = len(self.bids)
    while k >= -bids_size:
      p = self.bids[k][0]
      thresh = p - interval
      summed = 0
      while k >= -bids_size and self.bids[k][0] > thresh:
        summed += self.bids[k][1]
        k -= 1
      groupped_bids.append(summed)
    # Asks
    k = -1
    groupped_asks = []
    asks_size = len(self.asks)
    while k >= -asks_size:
      p = self.asks[k][0]
      thresh = p + interval
      summed = 0
      while k >= -asks_size and self.asks[k][0] < thresh:
        summed += self.asks[k][1]
        k -= 1
      groupped_asks.append(summed)
    return groupped_bids, groupped_asks
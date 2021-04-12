struct Candle {
  Candle() : o(0), h(0), l(0), c(0) {}

  inline double GetOpen() const { return o; }
  inline double GetHigh() const { return h; }
  inline double GetLow() const { return l; }
  inline double GetClose() const { return c; }
  inline double GetVolume() const { return vol; }

  void Reset(double open_price, double volume) {
    o = open_price;
    h = open_price;
    l = open_price;
    c = open_price;
    vol = volume;
  }

  void Add(double val, double volume) {
    h = std::max(val, h);
    l = std::min(val, l);
    c = val;
    vol += volume;
  }

private:
  double o;
  double h;
  double l;
  double c;
  double vol;
};
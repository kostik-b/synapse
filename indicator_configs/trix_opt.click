
input_device          :: FromDevice (eth0)

//symbol_filter         :: MsgFilterSymbol (ALLOW_SYMBOLS "LLOYl BARCl")

//trade_processor       :: TradeProcessor (AGGREGATION_INTERVAL_SEC 10, SYMBOLS_ROUTING "LLOYl 0 BARCl 1")
trade_processor       :: TradeProcessor (AGGREGATION_INTERVAL_SEC 10, SYMBOLS_ROUTING "BPl 0", DEBUG false)

ewma_1, ewma_2, ewma_3  :: EwmaIncremental (ALPHA_PERIODS 13, BUF_SIZE 13, DEBUG false, IN_PORTS 1, OP_MODE 2)

trix                  :: Trix (BUF_SIZE 13, DEBUG false, IN_PORTS 1, OP_MODE 2)

split_1               :: SourceSplit (CLOSE 0, DEBUG  false)

tstamp                :: Timestamper

stats                 :: StatPrinter


// my new attempt
input_device -> trade_processor[0] -> tstamp -> split_1[0] -> ewma_1 -> ewma_2 -> ewma_3 -> trix -> stats-> Discard;


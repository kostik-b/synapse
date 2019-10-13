
input_device          :: FromDevice (eth0)

//symbol_filter         :: MsgFilterSymbol (ALLOW_SYMBOLS "LLOYl BARCl")

//trade_processor       :: TradeProcessor (AGGREGATION_INTERVAL_SEC 10, SYMBOLS_ROUTING "LLOYl 0 BARCl 1")
trade_processor         :: TradeProcessor (AGGREGATION_INTERVAL_SEC 10, SYMBOLS_ROUTING "BPl 0", DEBUG false)

split_1                 :: SourceSplit (HIGH 0, LOW 1, CLOSE 2, DEBUG  false)

tee_high, tee_low,
  tee_sum_tr            :: ReuseTee

vmu                     :: Vmu (DEBUG false, OP_MODE 2)
vmd                     :: Vmd (DEBUG false, OP_MODE 2)

tr                      :: NewTrueRange (DEBUG false, OP_MODE 2)

sum_vmu, sum_vmd, sum_tr:: Sum (DEBUG false, OP_MODE 2, SUM_PERIODS 13, BUF_SIZE 13)

viu                     :: Viu (DEBUG false, OP_MODE 2)
vid                     :: Vid (DEBUG false, OP_MODE 2)

tstamp                  :: Timestamper

stats                   :: StatPrinter

// -------------------

input_device -> trade_processor[0] -> tstamp -> split_1[0] -> tee_high; split_1[1] -> tee_low; split_1[2] -> [2]tr;

tee_high[0] -> [0]vmu; tee_high[1] -> [0]vmd; tee_high[2] -> [0]tr;
tee_low[0]  -> [1]vmu; tee_low[1]  -> [1]vmd; tee_low[2]  -> [1]tr;

vmu -> sum_vmu;
vmd -> sum_vmd;
tr  -> sum_tr -> tee_sum_tr;

sum_vmu -> [0]viu;
sum_vmd -> [0]vid;

tee_sum_tr[0] -> [1]viu;
tee_sum_tr[1] -> [1]vid -> stats -> Discard;



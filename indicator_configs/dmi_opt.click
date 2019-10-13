
input_device          :: FromDevice (eth0)

//symbol_filter         :: MsgFilterSymbol (ALLOW_SYMBOLS "LLOYl BARCl")

//trade_processor       :: TradeProcessor (AGGREGATION_INTERVAL_SEC 10, SYMBOLS_ROUTING "LLOYl 0 BARCl 1")
trade_processor         :: TradeProcessor (AGGREGATION_INTERVAL_SEC 10, SYMBOLS_ROUTING "BPl 0", DEBUG false)

split_1                 :: SourceSplit (HIGH 0, LOW 1, CLOSE 2, DEBUG  false)

tee_high, tee_low,
 tee_ewma_tr            :: ReuseTee

pdm                     :: Pdm (DEBUG false, OP_MODE 2)
ndm                     :: Ndm (DEBUG false, OP_MODE 2)

tr                      :: NewTrueRange (DEBUG false, OP_MODE 2)

ewma_pdm, ewma_ndm,
      ewma_tr, ewma_dx  :: EwmaIncremental (ALPHA_PERIODS 13, BUF_SIZE 13, DEBUG false, OP_MODE 2)

pdi                     :: Pdi (DEBUG false, OP_MODE 2)
ndi                     :: Ndi (DEBUG false, OP_MODE 2)

dx                      :: Dx (DEBUG false, OP_MODE 2)

tstamp                  :: Timestamper

stats                   :: StatPrinter

// -------------------

input_device -> trade_processor[0] -> tstamp -> split_1[0] -> tee_high; split_1[1] -> tee_low; split_1[2] -> [2]tr;

tee_high[0] -> [0]pdm; tee_high[1] -> [0]ndm; tee_high[2] -> [0]tr;
tee_low[0]  -> [1]pdm; tee_low[1]  -> [1]ndm; tee_low[2]  -> [1]tr;

pdm -> ewma_pdm;
ndm -> ewma_ndm;
tr  -> ewma_tr -> tee_ewma_tr;

ewma_pdm -> [0]pdi;
ewma_ndm -> [0]ndi;

tee_ewma_tr[0] -> [1]pdi;
tee_ewma_tr[1] -> [1]ndi;

pdi -> [0]dx;
ndi -> [1]dx;

dx -> ewma_dx -> stats -> Discard;




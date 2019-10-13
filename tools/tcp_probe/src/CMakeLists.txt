
PROJECT (TCP_PROBE)

EXECUTE_PROCESS (COMMAND scp ${CMAKE_CURRENT_SOURCE_DIR}/runTcpReceiver.sh  ${CMAKE_CURRENT_SOURCE_DIR}/runMsgSender.sh ${CMAKE_CURRENT_SOURCE_DIR}/runUdpStats.sh ${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE (tcpReceiver   TcpReceiverApp.cc)
ADD_EXECUTABLE (msgSender     MsgSenderApp.cc)

TARGET_LINK_LIBRARIES (msgSender rt)
ADD_EXECUTABLE (udpReceiver   UdpReceiverApp.cc)

ADD_EXECUTABLE (udpStats UdpStats.cc)
TARGET_LINK_LIBRARIES (udpStats libev.a)

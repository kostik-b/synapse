cd ../shmq

insmod ./shmq.ko

cd -

insmod ./packet_capture.ko s_netif_param="eth1"

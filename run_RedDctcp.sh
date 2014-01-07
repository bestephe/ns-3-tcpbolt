#!/bin/bash

# Vary the random number generators run
# --RngRun=1
# NS_GLOBAL_VALUE="RngRun=1"

if [ $# -ne 1 ] ; then
    echo "Usage: $0 <init_run>"
    exit 1
fi
init_run=$1

trap "kill 0; exit 0" SIGINT SIGTERM EXIT

function spawn_sim {
    for (( run=$init_run; run<$(($init_run + 500)); run++ ))
    do
        RunValue="RngRun="$run
        #echo $RunValue
        export NS_GLOBAL_VALUE=$RunValue
        ulimit -Sv 3500000 # 3.5GB memory limit
        #echo $1
        time ./waf --run "$1 --simprefix=$2"
    done
}

# DCTCP
spawn_sim "scratch/oldi_sim --red=1 --dctcp=1" RedDctcp &

# Lossless DCTCP
spawn_sim "scratch/oldi_sim --red=1 --dctcp=1 --lossless=1" RedLosslessDctcp &

# Large CWND DCTCP
spawn_sim "scratch/oldi_sim --red=1 --dctcp=1 --initCwnd=200" RedLCwndDctcp &

# Lossless Large CWND DCTCP
spawn_sim "scratch/oldi_sim --red=1 --dctcp=1 --lossless=1 --initCwnd=200" RedLosslessLCwndDctcp &


while true
do
    sleep 10
done

# PacketSpray Lossless Large CWND TCP
#spawn_sim "scratch/oldi_sim --pktspray=1 --dctcp=1 --lossless=1 --initCwnd=200 --noSlowStart=1 PacketSprayLosslessLCwndDctcp" &

#for (( run=$init_run; run<$(($init_run + 200)); run++ ))
#do
#    RunValue="RngRun="$run
#    echo $RunValue
#    export NS_GLOBAL_VALUE=$RunValue
#
#    # Normal TCP
#    time ./waf --run "scratch/oldi_sim VanillaTcp"
#
#    # Lossless Normal TCP
#    time ./waf --run "scratch/oldi_sim --lossless=1 LosslessVanillaTcp"
#
#    # Large CWND TCP
#    time ./waf --run "scratch/oldi_sim --initCwnd=200 --noSlowStart=1 LCwndTcp"
#
#    # Lossless Large CWND TCP
#    time ./waf --run "scratch/oldi_sim --lossless=1 --initCwnd=200  --noSlowStart=1 LosslessLCwndTcp"
#
#    # DCTCP
#    time ./waf --run "scratch/oldi_sim --dctcp=1 Dctcp"
#
#    # Lossless DCTCP
#    time ./waf --run "scratch/oldi_sim --dctcp=1 --lossless=1 LosslessDctcp"
#
#    # Large CWND DCTCP
#    time ./waf --run "scratch/oldi_sim --dctcp=1 --initCwnd=200 --noSlowStart=1 LCwndDctcp"
#
#    # Lossless Large CWND TCP
#    time ./waf --run "scratch/oldi_sim --dctcp=1 --lossless=1 --initCwnd=200 --noSlowStart=1 LosslessLCwndDctcp"
#
#    # Lossless Medium CWND TCP
#    time ./waf --run "scratch/oldi_sim --dctcp=1 --lossless=1 --initCwnd=100 LosslessMCwndDctcp"
#
#    # PacketSpray Lossless Large CWND TCP
#    time ./waf --run "scratch/oldi_sim --pktspray=1 --dctcp=1 --lossless=1 --initCwnd=200 --noSlowStart=1 PacketSprayLosslessLCwndDctcp"
#
#done

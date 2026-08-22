#!/bin/sh
set -eu

mkdir -p raw-demos
adb shell wm size 1280x720
adb shell wm density 320
adb install -r wegert.apk
adb shell settings put system show_touches 1
adb shell am start -W -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -n org.isomorphisms.wegert/android.app.NativeActivity
sleep 4

# Wegert is landscape. At 1280x720/320 dpi the clear button is centered
# near (1078, 560), the zero control near (65, 655), and the pole
# control near (168, 655).
adb shell input tap 1078 560
sleep 1

adb shell screenrecord --size 1280x720 --bit-rate 2500000 --time-limit 13 /sdcard/add-and-drag-zero-and-pole.mp4 &
first_recorder_pid=$!
sleep 1
adb shell input tap 65 655
sleep 0.6
adb shell input tap 340 360
sleep 1.2
adb shell input tap 168 655
sleep 0.6
adb shell input tap 900 400
sleep 1.2
adb shell input swipe 340 360 500 250 1100
sleep 1.2
adb shell input swipe 900 400 780 520 1100
sleep 2.2
wait "$first_recorder_pid"
adb pull /sdcard/add-and-drag-zero-and-pole.mp4 raw-demos/
adb exec-out screencap -p > raw-demos/add-and-drag-zero-and-pole-final.png

adb shell input tap 1078 560
sleep 1

adb shell screenrecord --size 1280x720 --bit-rate 2500000 --time-limit 17 /sdcard/add-and-drag-two-zeros-and-two-poles.mp4 &
second_recorder_pid=$!
sleep 1
adb shell input tap 65 655
sleep 0.5
adb shell input tap 300 280
sleep 0.8
adb shell input tap 420 520
sleep 1
adb shell input tap 168 655
sleep 0.5
adb shell input tap 940 280
sleep 0.8
adb shell input tap 850 500
sleep 1
adb shell input swipe 300 280 430 350 1000
sleep 0.7
adb shell input swipe 420 520 550 440 1000
sleep 0.7
adb shell input swipe 940 280 820 220 1000
sleep 0.7
adb shell input swipe 850 500 960 450 1000
sleep 2
wait "$second_recorder_pid"
adb pull /sdcard/add-and-drag-two-zeros-and-two-poles.mp4 raw-demos/
adb exec-out screencap -p > raw-demos/add-and-drag-two-zeros-and-two-poles-final.png

#!/bin/sh
set -eu

mkdir -p raw-demos
adb shell wm size 720x1280
adb shell wm density 320
adb install -r wegert.apk
adb shell settings put system show_touches 1
adb shell am start -W -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -n org.isomorphisms.wegert/android.app.NativeActivity
sleep 4

# Start from an empty function. At 720x1280/320 dpi the clear button
# is centered near (518, 1120), the zero control near (65, 1215),
# and the pole control near (168, 1215).
adb shell input tap 518 1120
sleep 1

adb shell screenrecord --size 720x1280 --bit-rate 2500000 --time-limit 13 /sdcard/add-and-drag-zero-and-pole.mp4 &
first_recorder_pid=$!
sleep 1
adb shell input tap 65 1215
sleep 0.6
adb shell input tap 220 460
sleep 1.2
adb shell input tap 168 1215
sleep 0.6
adb shell input tap 500 760
sleep 1.2
adb shell input swipe 220 460 350 360 1100
sleep 1.2
adb shell input swipe 500 760 420 900 1100
sleep 2.2
wait "$first_recorder_pid"
adb pull /sdcard/add-and-drag-zero-and-pole.mp4 raw-demos/

adb shell input tap 518 1120
sleep 1

adb shell screenrecord --size 720x1280 --bit-rate 2500000 --time-limit 17 /sdcard/add-and-drag-two-zeros-and-two-poles.mp4 &
second_recorder_pid=$!
sleep 1
adb shell input tap 65 1215
sleep 0.5
adb shell input tap 190 420
sleep 0.8
adb shell input tap 260 850
sleep 1
adb shell input tap 168 1215
sleep 0.5
adb shell input tap 520 430
sleep 0.8
adb shell input tap 470 820
sleep 1
adb shell input swipe 190 420 300 520 1000
sleep 0.7
adb shell input swipe 260 850 360 750 1000
sleep 0.7
adb shell input swipe 520 430 430 320 1000
sleep 0.7
adb shell input swipe 470 820 540 940 1000
sleep 2
wait "$second_recorder_pid"
adb pull /sdcard/add-and-drag-two-zeros-and-two-poles.mp4 raw-demos/

#!/bin/bash

PS3='Please enter your choice: '
options=(`adb shell su -c "cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"`)
valid_options=
for (( i = 0; i < $((${#options[@]} - 1)); i++ )) do
  valid_options[$i]=${options[$i]}
done

function change_scaling_governor() {
  toChange=$1

  origin=(`adb shell su -c "cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"`)
  echo "currently, scale_governor is $origin"

  if [[ "$toChange" = "$origin" ]]; then
    echo "scale_governor was already changed to $toChange"
    echo "check current frequency without changing scale_governor"
  else
    adb shell su -c "echo $toChange > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
    result=(`adb shell su -c "cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"`)
    echo "scale_governor is changed to $result"
  fi
}

select opt in "${valid_options[@]}"
do
case $opt in
  *)
    found=0
    for (( i = 0; i < ${#valid_options[@]}; i++ )) do
      if [ "$opt" = "${valid_options[i]}" ]; then
        change_scaling_governor $opt
        found=1
      fi
    done
    if [[ $found -eq 0 ]]; then
      echo "invalid choice, choose between 1 to ${#valid_options[@]}"
      echo "check current frequency without changing scale_governor"
    else
      break
    fi
    ;;
esac
done

while [ 1 ]; do
  adb shell su -c "cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq"
  sleep 1s
done

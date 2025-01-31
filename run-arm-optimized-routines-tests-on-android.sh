#!/bin/bash

# Copy the tests across.
adb sync

if tty -s; then
  green="\033[1;32m"
  red="\033[1;31m"
  plain="\033[0m"
else
  green=""
  red=""
  plain=""
fi

failures=0

check_failure() {
  if [ $? -eq 0 ]; then
    echo -e "${green}[PASS]${plain}"
  else
    failures=$(($failures+1))
    echo -e "${red}[FAIL]${plain}"
  fi
}

# Run the 32-bit tests.
if [ -e "$ANDROID_PRODUCT_OUT/data/nativetest/mathtest/mathtest" ]; then
  adb shell /data/nativetest/mathtest/mathtest \
    '$(ls /data/nativetest/mathtest/math/test/testcases/directed/* | egrep -v "(atan2|cosh|exp10)")'
  check_failure
fi

# Run the 64-bit tests.
if [ -e "$ANDROID_PRODUCT_OUT/data/nativetest64/mathtest/mathtest" ]; then
  adb shell /data/nativetest64/mathtest/mathtest \
    '$(ls /data/nativetest/mathtest/math/test/testcases/directed/* | egrep -v "(atan2|cosh|exp10)")'
  check_failure
fi

echo
echo "_________________________________________________________________________"
echo
if [ $failures -ne 0 ]; then
  echo -e "${red}FAILED${plain}: $failures"
fi
exit $failures

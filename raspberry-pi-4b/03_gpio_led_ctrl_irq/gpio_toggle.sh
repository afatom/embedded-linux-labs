#!/usr/bin/env bash

echo -e	"================================================================="
echo -e "Toggle LED connected to GPIO <X> via device node exported\n"
echo -e "by the GPIO driver to the sysfs"
echo -e "HW Used: Raspberry PI 4B bcm2711"
echo -e	"================================================================="

GPIO_PIN=$1
GPIO_REAL_PIN=""
GPIO_DEV_PATH=""


function usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

Options:
  -h, --help        Display this help message
  -g GPIO Index     Specify GPIO index
EOF
}

# Check for help option
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    usage
    exit 0
fi

# Check for missing arguments
if [ "$#" -lt 1 ]; then
    echo "Error: Missing arguments." >&2 # Print error to standard error
    usage
    exit 1
fi

# Cleanup on normal exit or error
function cleanup() {
    if [ -d $GPIO_DEV_PATH ]; then
        # Make sure led is switched OFF before closing the GPIO
        echo "0" > $GPIO_DEV_PATH/value
        echo "$GPIO_REAL_PIN" > /sys/class/gpio/unexport
    fi

    exit 0
}

# In newer Linux kernels (5.x and 6.x), the GPIOs are grouped into chips.
# On the Pi 4, the main GPIO controller is often mapped to a high base number
# to avoid conflicts with other internal controllers.
function get_gpio_offset() {
	declare -i gpio_offset
	gpio_offset=$(grep -l "bcm2711" /sys/class/gpio/gpiochip*/label | sed 's/[^0-9]//g')

	echo $gpio_offset
}

declare -i offset=$(get_gpio_offset)

GPIO_REAL_PIN=$(( $offset + $GPIO_PIN))

GPIO_DEV_PATH="/sys/class/gpio/gpio$GPIO_REAL_PIN"

# Trap signals like Ctrl+C
trap cleanup EXIT

# 1. Export if not already exported
if [ ! -d "$GPIO_DEV_PATH" ]; then
    echo "$GPIO_REAL_PIN" > /sys/class/gpio/export
fi

# 2. Set direction
echo out > "$GPIO_DEV_PATH/direction"

# 3. Blink loop
echo "Blinking LED on GPIO $GPIO_PIN..."
for i in {1..5}; do
	echo -ne "Current toggle count: $i\r"
    echo 1 > "$GPIO_DEV_PATH/value"
    sleep 0.300
    echo 0 > "$GPIO_DEV_PATH/value"
    sleep 0.300
done

# 4. Final Cleanup
cleanup

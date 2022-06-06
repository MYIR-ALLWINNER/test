#! /bin/sh

#for myd and myt- modfiy led statu 20191212
#
#置高是灭
#gpioset gpiochip0 14=1
#gpioset gpiochip3 13=1

#test
#mount -t vfat /dev/sda /mnt
#imagedir=/mnt/mfg-images
imagedir=/run/media/mmcblk1p5/mfgimages
. ${imagedir}/Manifest
tfafile=${imagedir}/${tfafile}
ubootfile=${imagedir}/${ubootfile}
bootfsfile=${imagedir}/${bootfsfile}
rootfsfile=${imagedir}/${rootfsfile}
vendorfsfile=${imagedir}/${vendorfsfile}
userfsfile=${imagedir}/${userfsfile}


echo "Update file list:"
echo "tfafile file: ${tfafile}"
echo "uboot file: ${ubootfile}"
echo "bootfs file: ${bootfsfile}"
echo "rootfs file: ${rootfsfile}"
echo "vendorfs file: ${vendorfsfile}"
echo "userfsfile file: ${userfsfile}"
echo "LED name: ${ledname}"

# if echo ${ledname} | egrep -q '^[0-9]+$'; then
	# echo "LED is GPIO"
	# echo ${ledname} > /sys/class/gpio/export
	# echo "out" > /sys/class/gpio/gpio${ledname}/direction
# else
	# echo "LED is name"
# fi

bootpart=""
vendorpart=""
rootpart=""
userpart=""

LED_PID=""
LED_BLUE=114
LED_RED=115


#置高是灭
#gpioset gpiochip0 14=1

#gpioset gpiochip3 13=1
update_start()
{
	echo "Update starting..."
	#echo heartbeat > /sys/class/leds/user/trigger
	while true;do
		echo "Updating..."
		sleep 1
		echo "Updating..."
		sleep 1
	done
}



update_success()
{
	kill $LED_PID
	# if echo ${ledname} | egrep -q '^[0-9]+$'; then
		# echo 0 > /sys/class/gpio/gpio${ledname}/value
	# else
		# echo none > /sys/class/leds/${ledname}/trigger
		# echo 1 > /sys/class/leds/${ledname}/brightness
	# fi
    echo 1 > /sys/class/leds/heartbeat/brightness
    echo 0 > /sys/class/leds/heartbeat/brightness    

    echo "************************************************"
    echo "********** System update successfully **********"
    echo "************************************************"
    echo
	while true;do
		echo "Update complete..."
		sleep 1
		echo "Update complete..."
		sleep 1
	done
	

}

update_fail()
{
	kill $LED_PID
	# if echo ${ledname} | egrep -q '^[0-9]+$'; then
		# echo 1 > /sys/class/gpio/gpio${ledname}/value
	# else
		# echo none > /sys/class/leds/${ledname}/trigger
		# echo 0 > /sys/class/leds/${ledname}/brightness
	# fi
	#gpioset gpiochip0 14=1
	#gpioset gpiochip3 13=1
	echo 1 > /sys/class/leds/heartbeat/brightness 	

	while true;do
		echo "Update failed..."
		sleep 1
		echo "Update failed..."
		sleep 1
	done
}

cmd_check()
{
	if [ $1 -ne 0 ];then
		echo "$2 failed!"
		echo
		update_fail
	fi
}

check_file()
{
	if [ ! -s $1 ];then
		echo "invalid imagefile $1"
		echo
		update_fail
	fi
}

if grep "root" /proc/cmdline > /dev/null;then
	echo
	echo "***********************************************"
	echo "*************    SYSTEM UPDATE    *************"
	echo "***********************************************"
	echo
else
	echo "Normal boot"
	exit 0
fi

update_start &
LED_PID=$!
# if echo ${ledname} | egrep -q '^[0-9]+$'; then
	# update_start_gpioled &
	# LED_PID=$!
# else
	# update_start_nameled &
	# LED_PID=$!
# fi


STEP=1
TOTAL_STEPS=8
if [ -s $datafile ];then
    TOTAL_STEPS=8
fi

echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Check image files in $imagedir ... "
check_file $tfafile
check_file $ubootfile
check_file $bootfsfile
check_file $rootfsfile
check_file $userfsfile
check_file $vendorfsfile
echo "OK"
echo

let STEP=STEP+1
echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Checking Rootfs device: "
DRIVE="/dev/mmcblk2"
echo OK
echo

# if grep "rootdev" /proc/cmdline > /dev/null;then
	# ROOTDEV=`awk 'BEGIN{RS=" "} /rootdev/ {print}' /proc/cmdline |awk -F '=' '{print $2}'`
	# if [ ! -z $ROOTDEV ];then
		# DRIVE=$ROOTDEV
	# fi
# fi
# echo "$DRIVE"
# echo

# let STEP=STEP+1
# echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Unmount all partitions in $DRIVE ... "
# umount ${DRIVE}* > /dev/null 2>&1
# echo OK
# echo

# dd if=/dev/zero of=${DRIVE} bs=1024 count=1 > /dev/null 2>&1
# cmd_check $? "Destroy partition table"

# SIZE=`fdisk -l $DRIVE | grep Disk | awk '{print $5}'`
# echo -n "DISK SIZE: $SIZE bytes"

# CYLINDERS=`echo $SIZE/255/63/512 | bc`
# echo "CYLINDERS: $CYLINDERS"
# echo

#let STEP=STEP+1
#echo "[ ${STEP} / ${TOTAL_STEPS} ] Re-partition device"
#umount ${DRIVE}* > /dev/null 2>&1
#{
#echo ,25,0x0C,*
#echo ,,,-
#} | sfdisk -D -H 255 -S 63 -C $CYLINDERS $DRIVE > /dev/null 2>&1
#} | sfdisk -D -H 255 -S 63 -C $CYLINDERS $DRIVE
#sfdisk --force ${DRIVE} << EOF
#1024,2M,0c
#3M,64M,83
#67M,83M,83
#150M,790M,83
#940M,1004M,83
#EOF

#cmd_check $? "Re-partition device"
#mdev -s > /dev/null 2>&1
#umount ${DRIVE}* > /dev/null 2>&1
#echo

let STEP=STEP+1
echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Write tf-a ..."
echo 0 > /sys/class/block/mmcblk2boot0/force_ro
echo 0 > /sys/class/block/mmcblk2boot1/force_ro
#cmd_check $? "Write tf-a to eMMC"
#dd if=/mnt/tf-a-stm32mp157c-dk2-trusted.stm32 of=/dev/mmcblk2boot0 conv=fsync > /dev/null 2>&1
#dd if=/mnt/tf-a-stm32mp157c-dk2-trusted.stm32 of=/dev/mmcblk2boot1 conv=fsync > /dev/null 2>&1
#echo 1 > /sys/class/block/mmcblk2boot0/force_ro
#echo 1 > /sys/class/block/mmcblk2boot1/force_ro
dd if=${tfafile} of=/dev/mmcblk2boot0 bs=1M conv=fsync > /dev/null 2>&1
dd if=${tfafile} of=/dev/mmcblk2boot1 bs=1M conv=fsync > /dev/null 2>&1
#cmd_check $? "Update tf-a"
echo 1 > /sys/class/block/mmcblk2boot0/force_ro
echo 1 > /sys/class/block/mmcblk2boot1/force_ro
sleep 1
sync
echo OK
echo

let STEP=STEP+1
echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Write uboot ..."
cmd_check $? "Write uboot to eMMC"
dd if=${ubootfile} of=/dev/mmcblk2p1 bs=1M conv=fsync > /dev/null 2>&1
cmd_check $? "Update uboot"
sleep 1
sync
echo OK
echo

mmc bootpart enable 1 1 /dev/mmcblk2
cmd_check $? "Enable boot partion 1 to boot"

#boot_config=`mmc extcsd read /dev/mmcblk1 | grep PARTITION_CONFIG | grep 0x48`

#if [ "X$boot_config" = "X" ]; then
#    echo "eMMC Boot Config failed"
#	update_fail
#fi


#bootfs
#let STEP=STEP+1
#echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Formating bootfs partition ... "
#umount ${DRIVE}* > /dev/null 2>&1
#if [ -b ${DRIVE}2 ]; then
#	mkfs.ext4 -F -L "bootfs" ${DRIVE}2 > /dev/null 2>&1
#	cmd_check $? "Formating bootfs partition"
#	bootpart=`basename ${DRIVE}2`
#else
#	if [ -b ${DRIVE}p2 ]; then
#		mkfs.ext4 -F -L "bootfs" ${DRIVE}p2 > /dev/null 2>&1
#		cmd_check $? "Formating bootfs partition"
#		bootpart=`basename ${DRIVE}p2`
#	else
#		echo "Cant find bootfs partition in ${DRIVE}2 or ${DRIVE}p2"
#		echo
#		update_fail
#	fi
#fi
#echo OK
#echo

let STEP=STEP+1
#umount /dev/${bootpart}
echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Copy bootfs files ... "
mkdir -p /run/media/${bootpart}
#mount -t ext4 -o rw,noatime,nodiratime /dev/${bootpart} /run/media/${bootpart}
dd if=${bootfsfile} of=/dev/mmcblk2p2 bs=8M conv=fsync > /dev/null 2>&1
#cmd_check $? "mount ${bootpart} failed"
#tar xvf ${bootpart} -C /mnt/mfg-images/${bootpart}
# mount -t ext4 -o rw,noatime,nodiratime /dev/${bootpart} /run/media/${bootpart}
# cmd_check $? "Extra ${bootfsfile}"
sync
#umount /dev/${bootpart}
echo OK
echo


#vendorfs
#let STEP=STEP+1
#echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Formating vendorfs partition ... "
#umount ${DRIVE}* > /dev/null 2>&1
#if [ -b ${DRIVE}3 ]; then
#	mkfs.ext4 -F -L "bootfs" ${DRIVE}3 > /dev/null 2>&1
	# cmd_check $? "Formating vendorfs partition"
	# vendorpart=`basename ${DRIVE}3`
# else
	# if [ -b ${DRIVE}p3 ]; then
		# mkfs.ext4 -F -L "vendorfs" ${DRIVE}p3 > /dev/null 2>&1
		# cmd_check $? "Formating vendorfs partition"
		# vendorpart=`basename ${DRIVE}p3`
	# else
		# echo "Cant find vendorfs partition in ${DRIVE}3 or ${DRIVE}p3"
		# echo
		# update_fail
	# fi
# fi
# echo OK
# echo

let STEP=STEP+1
#umount /dev/${vendorpart}
echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Copy vendorfs files ... "
mkdir -p /run/media/${vendorpart}
#mount -t ext4 -o rw,noatime,nodiratime /dev/${vendorpart} /run/media/${vendorpart}
dd if=${vendorfsfile} of=/dev/mmcblk2p3 bs=8M conv=fsync > /dev/null 2>&1
#cmd_check $? "mount ${vendorpart} failed"
#tar xvf ${vendorpart} -C /run/media/${vendorpart}
# mount -t ext4 -o rw,noatime,nodiratime /dev/${vendorpart} /run/media/${vendorpart}
# cmd_check $? "Extra ${bootfsfile}"
sync
#umount /dev/${vendorpart}
echo OK
echo

#rootfs
# let STEP=STEP+1
# echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Formating rootfs partition ... "
# umount ${DRIVE}* > /dev/null 2>&1
# if [ -b ${DRIVE}4 ]; then
	# mkfs.ext4 -F -L "rootfs" ${DRIVE}4 > /dev/null 2>&1
	# cmd_check $? "Formating rootfs partition"
	# rootpart=`basename ${DRIVE}4`
# else
	# if [ -b ${DRIVE}p4 ]; then
		# mkfs.ext4 -F -L "rootfs" ${DRIVE}p4 > /dev/null 2>&1
		# cmd_check $? "Formating rootfs partition"
		# rootpart=`basename ${DRIVE}p4`
	# else
		# echo "Cant find rootfs partition in ${DRIVE}4 or ${DRIVE}p4"
		# echo
		# update_fail
	# fi
# fi
# echo OK
# echo
# sleep 3

let STEP=STEP+1
#umount /dev/${rootpart}
echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Copy rootfs files ... "
mkfs.ext4 -F -L "rootfs" /dev/mmcblk2p4 > /dev/null 2>&1
mkdir -p /run/media/${rootpart}
#mount -t ext4 -o rw,noatime,nodiratime /dev/${rootpart} /run/media/${rootpart}
dd if=${rootfsfile} of=/dev/mmcblk2p4 bs=10M conv=fsync > /dev/null 2>&1
#dd if=st-image-weston-openstlinux-weston-stm32mp1.ext4 of=/dev/mmcblk2p4 conv=fsync > /dev/null 2>&1
#cmd_check $? "mount ${rootpart} failed"
#tar xvf ${rootfsfile} -C /run/media/${rootpart}
# mount -t ext4 -o rw,noatime,nodiratime /dev/${rootpart} /run/media/${rootpart}
# cmd_check $? "Extra ${rootfsfile}"
sync
#umount /dev/${rootpart}
echo OK
echo

#userfs
# let STEP=STEP+1
# echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Formating userfs partition ... "
# umount ${DRIVE}* > /dev/null 2>&1
# if [ -b ${DRIVE}5 ]; then
	# mkfs.ext4 -F -L "userfs" ${DRIVE}5 > /dev/null 2>&1
	# cmd_check $? "Formating userfs partition"
	# userpart=`basename ${DRIVE}5`
# else
	# if [ -b ${DRIVE}p5 ]; then
		# mkfs.ext4 -F -L "userfs" ${DRIVE}p5 > /dev/null 2>&1
		# cmd_check $? "Formating userfs partition"
		# userpart=`basename ${DRIVE}p5`
	# else
		# echo "Cant find userfs partition in ${DRIVE}5 or ${DRIVE}p5"
		# echo
		# update_fail
	# fi
# fi
# echo OK
# echo

let STEP=STEP+1
#umount /dev/${userpart}
echo -n "[ ${STEP} / ${TOTAL_STEPS} ] Copy userfs files ... "
mkfs.ext4 -F -L "userfs" /dev/mmcblk2p5 > /dev/null 2>&1
mkdir -p /run/media/${userpart}
#mount -t ext4 -o rw,noatime,nodiratime /dev/${userpart} /run/media/${userpart}
dd if=${userfsfile} of=/dev/mmcblk2p5 bs=10M conv=fsync > /dev/null 2>&1
#cmd_check $? "mount ${userpart} failed"
#tar xvf ${userfsfile} -C /run/media/${userpart}
# mount -t ext4 -o rw,noatime,nodiratime /dev/${userpart} /run/media/${userpart}
# cmd_check $? "Extra ${rootfsfile}"
sync
#umount /dev/${userpart}
echo OK
echo



update_success


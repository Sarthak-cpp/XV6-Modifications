!/bin/bash

submission=$1
mkdir ./test_dir

echo $submission

fileNameRegex="assignment1_easy_[0-9]{4}[A-Z]{2}.[0-9]{4}.tar.gz"

if ! [[ $submission =~ $fileNameRegex ]]; then
	echo "File doesn't match the naming convention"
	exit
fi

echo "Setting the test directory"

tar -xzvf $submission -C ./test_dir
cp arr assig1_1.c assig1_2.c assig1_3.c assig1_4.c assig1_5.c assig1_6.c assig1_7.c out* *.sh ./test_dir
cd ./test_dir

FILE=report.pdf
if ! [[ -f "$FILE" ]]; then
	echo "Report does not exist"
	exit
fi

echo "Executing the test cases"

pkill qemu-system-x86
pkill qemu-system-i386
make clean
make fs.img
make

#make qemu 
echo "Running..1"
./test_assig1.sh assig1_1|grep -i 'sys_'|sed 's/$ //g'|sort > res_assig1_1

echo "Running..2"
./test_assig1.sh assig1_2|grep -i 'sys_'|sed 's/$ //g'|sort > res_assig1_2

echo "Running..3"
./test_assig1.sh assig1_3|grep -i Sum|sed 's/$ //g'|sort > res_assig1_3

echo "Running..4"
./test_assig1.sh assig1_4|grep -i Sum|sed 's/$ //g'|sort > res_assig1_4

echo "Running..5"
./test_assig1.sh assig1_5|grep -i pid|sed 's/$ //g'|sort > res_assig1_5

echo "Running..6"
./test_assig1.sh assig1_6|grep -i pid|sed 's/$ //g'|sort > res_assig1_6

echo "Running..7"
./test_assig1.sh assig1_7|grep -i 'PARENT\|CHILD'|sed 's/$ //g'|sort> res_assig1_7

echo "Running..8 (this will take 10 seconds)"
./test_assig1_long.sh assig1_8 0 arr|grep -i 'Sum of array'|sed 's/$ //g'|sort> res_assig1_8



check_test=8
total_test=0

for ((t=1;t<=$check_test;++t))
do
	echo -n "Test #${t}: "

	# NOTE: we are doing case insensitive matching.  If this is not what you want,
	# just remove the "-i" flag
	if diff -iZ <(sort out_assig1_$t) <(sort res_assig1_$t) > /dev/null
	then
		echo -e "\e[0;32mPASS\e[0m"
		((total_test++))
	else
		echo -e "\e[0;31mFAIL\e[0m"
	fi
done
echo "$total_test" test cases passed

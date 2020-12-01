#!/bin/sh

if [ ! -d "datasets" ]; then
    echo "Missing datasets.  Download and extract https://pix.jar.jp/palmtrie/datasets.tar.gz"
    exit 1
fi

./configure
if [ $? -ne 0 ]; then
    echo "Failed to run ./configure" >& 2
    exit 1
fi

make clean all CFLAGS="-O3 -mpopcnt -DPALMTRIE_SHORT=1"
if [ $? -ne 0 ]; then
    echo "Failed to build" >& 2
    exit 1
fi

CWD=`pwd`
DIR=`mktemp -d -t palmtrie-evalXXXXX`
if [ $? -ne 0 ]; then
    echo "Failed to create temporary file" >& 2
    exit 1
fi
echo "Evaluation Directiory: $DIR"


cd $DIR

mkdir tests
cp $CWD/tests/traffic.sfl2 tests/
cp $CWD/tests/acl-1000.ross tests/

## Campus network policy
mkdir campus
mkdir campus/scanning
mkdir campus/random
for i in `seq 0 1 16`; do
     echo "Evaluating acl-D${i} performance..."
     $CWD/palmtrie_eval_acl $CWD/datasets/tcam/acl-D${i}.tcam popmtpt-ross > campus/scanning/result.acl-D${i}.txt
     $CWD/palmtrie_eval_acl $CWD/datasets/tcam/acl-D${i}.tcam popmtpt-sfl > campus/random/result.acl-D${i}.txt
done

## ClassBench
mkdir classbench
for d in "acl1" "fw2" "ipc2"; do for n in "1k" "10k" "50k" "100k" "200k" "500k"; do
     echo "Evaluating ClassBench ${d}_seed_${n} performance..."
    cp $CWD/datasets/traffic/${d}_seed_${n}_trace.txt tests/traffic.tmp
    $CWD/palmtrie_eval_acl $CWD/datasets/tcam/classbench-wo-flags.${d}_seed_${n}.tcam popmtpt-traffic > classbench/result.${d}_seed_${n}.txt
done; done

echo "All done!  The evaluation results are saved at $DIR"



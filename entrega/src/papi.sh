#! /bin/bash
DATASET="LARGE"

gcc  -I utilities/ -I /usr/local/include/ utilities/polybench.c cholesky_developed.c /usr/local/lib/libpapi.a -lm -D${DATASET}_DATASET -o cholesky_developed.out
gcc  -I utilities/ -I /usr/local/include/ utilities/polybench.c cholesky_pthread.c /usr/local/lib/libpapi.a -pthread -lm -D${DATASET}_DATASET -o cholesky_pthread.out
gcc  -I utilities/ -I /usr/local/include/ utilities/polybench.c cholesky_omp.c /usr/local/lib/libpapi.a -fopenmp -lm -D${DATASET}_DATASET -o cholesky_omp.out

PROGRAM="cholesky"

echo "PAPI EVENTS"

# SEQUENTIAL CHOLESKY
# echo "SEQUENTIAL"
file="./papidata/${PROGRAM}_developed_sequential.data"
if [ -f "$file" ]
then
	rm $file
fi
for i in {1..11}
do
	 echo "SEQUENTIAL DEVELOPED - EXECUTION $i"
   if [[ $i == 1 ]]; then
     ./${PROGRAM}_developed.out 1 > /dev/null
   else
    # ./${PROGRAM}_developed.out $MSIZE 2 >> testfile.data
     ./${PROGRAM}_developed.out 1 >> ./papidata/${PROGRAM}_developed_sequential.data 2>&1
  fi
done
for i in {12..21}
do
	 echo "SEQUENTIAL DEVELOPED - EXECUTION $i"
     ./${PROGRAM}_developed.out 2 >> ./papidata/${PROGRAM}_developed_sequential.data 2>&1
done


# OMP CHOLESKY

# echo "OMP FOR $t THREADS"
file="./papidata/${PROGRAM}_omp_8.data"
if [ -f "$file" ]
then
	rm $file
fi
for i in {1..11}
do
	 echo "OMP FOR 8 THREADS - EXECUTION $i"
 if [[ $i == 1 ]]; then
   ./${PROGRAM}_omp.out 8 1 > /dev/null
 else
  # ./${PROGRAM}.out $MSIZE 2 >> testfile.data
   ./${PROGRAM}_omp.out 8 1 >> ./papidata/${PROGRAM}_omp_8.data 2>&1
fi
done
for i in {12..21}
do
	 echo "OMP FOR 8 THREADS - EXECUTION $i"
   ./${PROGRAM}_omp.out 8 2 >> ./papidata/${PROGRAM}_omp_8.data 2>&1
done


# PTHREAD CHOLESKY

file="./papidata/${PROGRAM}_pthread_8.data"
if [ -f "$file" ]
then
	rm $file
fi
# echo "PTHREAD FOR $t THREADS"
for i in {1..11}
do
	echo "PTHREAD FOR $t THREADS - EXECUTION $i"
	if [[ $i == 1 ]]; then
		./${PROGRAM}_pthread.out 8 1 > /dev/null
	else
		# ./${PROGRAM}.out $MSIZE 2 >> testfile.data
		./${PROGRAM}_pthread.out 8 1 >> ./papidata/${PROGRAM}_pthread_8.data 2>&1
	fi
done
for i in {12..21}
do
	echo "PTHREAD FOR $t THREADS - EXECUTION $i"
		./${PROGRAM}_pthread.out 8 2 >> ./papidata/${PROGRAM}_pthread_8.data 2>&1
done

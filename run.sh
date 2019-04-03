#!/bin/bash

EXEFILE=fileconvert
LOGFILE=test.log
INPUTFILE=stdin.txt
OUTPUTFILE=stdout.txt

rm -f $EXEFILE *.txt *~ a.out core.* $LOGFILE
if [ "x$1" = "xrelease" ]; then
    gcc -O2 fileconvert.cpp -o $EXEFILE
else
    gcc -g3 fileconvert.cpp -o $EXEFILE
fi
chmod +x $EXEFILE

function printmsg(){
    echo "" >>  $LOGFILE
    echo "******************** $1 *******************" >>  $LOGFILE
}

printmsg "test start"
printmsg "test help message"

./$EXEFILE 2>&1 >> $LOGFILE
ls -l *.txt >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test empty file"
echo "" > $INPUTFILE
./$EXEFILE "2"  2>&1 >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test one column file"
echo "12345" > $INPUTFILE
echo "abcde" >> $INPUTFILE
cat $INPUTFILE >> $LOGFILE
./$EXEFILE "5" 2>&1 >> $LOGFILE
cat $INPUTFILE >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test two columns file" 
echo "112" > $INPUTFILE
echo "121" >> $INPUTFILE
cat $INPUTFILE >> $LOGFILE
./$EXEFILE "1,2" 2>&1 >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
./$EXEFILE "2,1" 2>&1 >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test four columns file"
echo "1aa123" > $INPUTFILE
echo "123AA1" >> $INPUTFILE
cat $INPUTFILE >> $LOGFILE
./$EXEFILE "1,2,3" 2>&1 >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
./$EXEFILE "2,1,3" 2>&1 >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
./$EXEFILE "3,2,1" 2>&1 >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test source file"
rm -rf *.txt >>  $LOGFILE
echo "1aa123" > source.txt
cat source.txt >> $LOGFILE
./$EXEFILE "2,2,2" source.txt 2>&1 >> $LOGFILE
ls *.txt >> $LOGFILE
cat stdout.txt >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test source and target file"
rm -rf *.txt >>  $LOGFILE
echo "1aa123" > source.txt
cat source.txt >> $LOGFILE
rm -rf stdout.txt >>  $LOGFILE
./$EXEFILE "2,2,2" source.txt target.txt 2>&1 >> $LOGFILE
ls *.txt >> $LOGFILE
cat target.txt >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test source and target file"
rm -rf *.txt >>  $LOGFILE
echo "1aa123" > source.txt
echo "1aa123" >> source.txt
cat source.txt >> $LOGFILE
./$EXEFILE "3,2,1" source.txt target.txt "|" 2>&1 >> $LOGFILE
ls *.txt >> $LOGFILE
cat target.txt >> $LOGFILE
echo "" >>  $LOGFILE

# rm *.txt

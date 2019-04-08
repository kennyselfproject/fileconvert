#!/bin/bash

EXEFILE=fileconvert
LOGFILE=test.log
EXECUTELOG=execute.log
INPUTFILE=stdin.txt
OUTPUTFILE=stdout.txt

rm -f $EXEFILE *.txt *~ a.out core.* $EXECUTELOG $LOGFILE *.tar.gz
if [ "x$1" = "xrelease" ]; then
    gcc -O2 fileconvert.cpp -o $EXEFILE
    tar zcvf $EXEFILE.tar.gz $EXEFILE
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
ls -l | grep "\.txt" 2>&1 >> $LOGFILE

printmsg "test empty file"
echo "" > $INPUTFILE
echo "$INPUTFILE" >> $LOGFILE
cat $INPUTFILE >> $LOGFILE
./$EXEFILE "2"  2>&1 >> $EXECUTELOG
echo "$OUTPUTFILE" >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test one column file"
echo "12345" > $INPUTFILE
echo "abcde" >> $INPUTFILE
echo "$INPUTFILE" >> $LOGFILE
cat $INPUTFILE >> $LOGFILE
./$EXEFILE "5" 2>&1 >> $EXECUTELOG
echo "$OUTPUTFILE" >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test two columns file" 
echo "112" > $INPUTFILE
echo "121" >> $INPUTFILE
echo "$INPUTFILE" >> $LOGFILE
cat $INPUTFILE >> $LOGFILE
./$EXEFILE "1,2" 2>&1 >> $EXECUTELOG
echo "$OUTPUTFILE" >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
./$EXEFILE "2,1" 2>&1 >> $EXECUTELOG
echo "$OUTPUTFILE" >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test four columns file"
echo "1aa123" > $INPUTFILE
echo "123AA1" >> $INPUTFILE
echo "$INPUTFILE" >> $LOGFILE
cat $INPUTFILE >> $LOGFILE
./$EXEFILE "1,2,3" 2>&1 >> $EXECUTELOG
echo "$OUTPUTFILE" >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
./$EXEFILE "2,1,3" 2>&1 >> $EXECUTELOG
echo "$OUTPUTFILE" >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
./$EXEFILE "3,2,1" 2>&1 >> $EXECUTELOG
echo "$OUTPUTFILE" >> $LOGFILE
cat $OUTPUTFILE >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test source file"
rm -rf *.txt >>  $LOGFILE
echo "1aa123" > source.txt
echo "source.txt" >> $LOGFILE
cat source.txt >> $LOGFILE
./$EXEFILE "2,2,2" source.txt 2>&1 >> $EXECUTELOG
ls *.txt >> $LOGFILE
echo "stdout.txt" >> $LOGFILE
cat stdout.txt >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test source and target file"
rm -rf *.txt >>  $LOGFILE
echo "1aa123" > source.txt
echo "source.txt" >> $LOGFILE
cat source.txt >> $LOGFILE
rm -rf stdout.txt >>  $LOGFILE
./$EXEFILE "2,2,2" source.txt target.txt 2>&1 >> $EXECUTELOG
ls *.txt >> $LOGFILE
echo "target.txt" >> $LOGFILE
cat target.txt >> $LOGFILE
echo "" >>  $LOGFILE

printmsg "test source and target file"
rm -rf *.txt >>  $LOGFILE
echo "1aa123" > source.txt
echo "1aa123" >> source.txt
echo "source.txt" >> $LOGFILE
cat source.txt >> $LOGFILE
./$EXEFILE "3,2,1" source.txt target.txt "|" 2>&1 >> $EXECUTELOG
ls *.txt >> $LOGFILE
echo "target.txt" >> $LOGFILE
cat target.txt >> $LOGFILE
echo "" >>  $LOGFILE

rm *.txt
diff expect.log $LOGFILE > diff.log
echo "Finished!"
cat diff.log
